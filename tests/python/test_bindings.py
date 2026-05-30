"""pytest suite for the AsyncioDAT bindings, via the `asynciodat` test extension.

The extension compiles the real operator sources and creates one operator
instance at import time (which initializes the asyncio event loop), so the
binding functions operate on a live instance — no TouchDesigner required.

There is no per-frame cooking here, so the loop is advanced by calling
poll_event_loop() in a loop — the headless analog of TouchDesigner frames.
Timer-based coroutines (asyncio.sleep with a real delay) only make progress as
wall-clock time passes between polls, exactly as they would across frames, so
the timing/ordering behavior the integration suite checks inside TouchDesigner
is checked here too, just driven manually.
"""

import time
import asyncio

import pytest

import asynciodat


# --- helpers ---------------------------------------------------------------

def poll(n):
    """Poll the loop n times — enough to drain tasks that only await sleep(0)."""
    for _ in range(n):
        asynciodat.poll_event_loop()


def pump_until(predicate, timeout=5.0, tick=0.001):
    """Poll the loop until predicate() is true or timeout elapses.

    Sleeps a hair between polls so real wall-clock time advances and
    timer-based coroutines (asyncio.sleep with a delay) can mature — this is
    what frames do in TouchDesigner.
    """
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        asynciodat.poll_event_loop()
        if predicate():
            return True
        time.sleep(tick)
    return predicate()


@pytest.fixture(autouse=True)
def clean_plugins():
    """Keep tests order-independent by clearing plugin state around each test.

    The single operator instance is created and initialized at import and lives
    for the whole session; the loop is never torn down here (shutdown releases
    the active instance, and the binding layer has no way to re-register it —
    that lifecycle belongs to TouchDesigner, not this headless module).
    """
    asynciodat.clear_plugins()
    yield
    asynciodat.clear_plugins()


# --- lifecycle / basics ----------------------------------------------------

def test_initialized_at_import():
    assert asynciodat.is_running() is True
    assert asynciodat.asyncio_initialized() is True
    assert asynciodat.loop_running() is True


def test_event_loop_is_a_real_loop():
    loop = asynciodat.get_event_loop()
    assert loop is not None
    assert isinstance(loop, asyncio.AbstractEventLoop)


def test_callback_count_is_int():
    count = asynciodat.get_callback_count()
    assert isinstance(count, int)
    assert count >= 0


# --- task scheduling -------------------------------------------------------

def test_add_task_runs_when_polled():
    result = {}

    async def job():
        await asyncio.sleep(0)
        result["value"] = 21 + 21

    assert asynciodat.add_task(job()) is True
    poll(10)
    assert result.get("value") == 42


def test_add_task_rejects_non_coroutine():
    # ensure_future on a non-awaitable fails; the binding reports False rather
    # than raising into Python.
    assert asynciodat.add_task(123) is False
    assert asynciodat.add_task("not a coroutine") is False


def test_run_coroutine_returns_value():
    async def compute():
        await asyncio.sleep(0)
        return 7

    assert asynciodat.run_coroutine(compute()) == 7


def test_create_task_returns_task():
    async def noop():
        await asyncio.sleep(0)

    task = asynciodat.create_task(noop())
    assert isinstance(task, asyncio.Task)
    poll(10)
    assert task.done()


def test_create_task_result_available_after_polling():
    async def compute():
        await asyncio.sleep(0.02)
        return 99

    task = asynciodat.create_task(compute())
    assert pump_until(lambda: task.done())
    assert task.result() == 99


# --- deferral / ordering / concurrency over polls --------------------------

def test_deferred_task_completes_only_after_delay():
    """A timer-based coroutine does not complete on the first poll; it lands
    later, once enough wall-clock time has passed (the headless analog of a
    task resolving N frames later)."""
    done = []

    async def job():
        await asyncio.sleep(0.05)
        done.append(1)

    asynciodat.add_task(job())
    asynciodat.poll_event_loop()
    assert done == []  # not on the scheduling poll

    assert pump_until(lambda: done == [1])


def test_completion_order_by_delay():
    """Tasks complete in delay order, not scheduling order."""
    order = []

    async def task(name, delay):
        await asyncio.sleep(delay)
        order.append(name)

    asynciodat.add_task(task("c", 0.06))
    asynciodat.add_task(task("a", 0.02))
    asynciodat.add_task(task("b", 0.04))

    assert pump_until(lambda: len(order) == 3)
    assert order == ["a", "b", "c"]


def test_fifo_order_same_delay():
    """Same (zero) delay: tasks complete in the order they were scheduled."""
    order = []

    async def task(name):
        await asyncio.sleep(0)
        order.append(name)

    for name in ("first", "second", "third"):
        asynciodat.add_task(task(name))
    poll(10)
    assert order == ["first", "second", "third"]


def test_tasks_run_concurrently_not_serially():
    """Interleaving proves tasks share the loop rather than running one to
    completion before the next starts: every task reaches its 'start' before
    any task reaches its 'end'."""
    log = []

    async def worker(i):
        log.append((i, "start"))
        await asyncio.sleep(0)
        log.append((i, "mid"))
        await asyncio.sleep(0)
        log.append((i, "end"))

    for i in range(3):
        asynciodat.add_task(worker(i))
    assert pump_until(lambda: len(log) == 9)

    first_end = min(k for k, (_, phase) in enumerate(log) if phase == "end")
    last_start = max(k for k, (_, phase) in enumerate(log) if phase == "start")
    assert last_start < first_end, f"serialized, not concurrent: {log}"


def test_exception_in_task_does_not_break_loop():
    """A task that raises is isolated: a sibling still completes, and the loop
    is still usable for new work afterward."""
    survived = []

    async def boom():
        await asyncio.sleep(0.01)
        raise ValueError("intentional failure")

    async def survivor():
        await asyncio.sleep(0.02)
        survived.append("ok")

    asynciodat.add_task(boom())
    asynciodat.add_task(survivor())
    assert pump_until(lambda: survived == ["ok"])

    # Loop still works after a task raised.
    after = []

    async def later():
        await asyncio.sleep(0)
        after.append(1)

    asynciodat.add_task(later())
    poll(10)
    assert after == [1]


def test_cancelled_task_does_not_run():
    state = []

    async def action():
        await asyncio.sleep(0.05)
        state.append("ran")

    task = asynciodat.create_task(action())
    task.cancel()
    assert pump_until(lambda: task.done())
    assert task.cancelled()
    assert state == []


# --- plugins ---------------------------------------------------------------

def test_plugin_roundtrip():
    assert asynciodat.has_plugin("x") is False

    assert asynciodat.set_plugin("x", {"a": 1}) is True
    assert asynciodat.has_plugin("x") is True
    assert asynciodat.get_plugin("x") == {"a": 1}
    assert "x" in list(asynciodat.plugin_names())

    assert asynciodat.del_plugin("x") is True
    assert asynciodat.has_plugin("x") is False
    assert asynciodat.get_plugin("x") is None


def test_get_plugin_returns_same_object():
    """The stored object is the same identity coming back (the binding holds a
    real reference, not a copy)."""
    sentinel = object()
    asynciodat.set_plugin("s", sentinel)
    assert asynciodat.get_plugin("s") is sentinel


def test_plugin_survives_dropping_local_reference():
    """Dropping the caller's reference doesn't free the plugin — the operator
    keeps it alive."""
    class Thing:
        marker = "kept"

    asynciodat.set_plugin("t", Thing())
    # No local reference remains; the operator must still own it.
    assert asynciodat.get_plugin("t").marker == "kept"


def test_set_plugin_overwrites():
    asynciodat.set_plugin("k", 1)
    asynciodat.set_plugin("k", 2)
    assert asynciodat.get_plugin("k") == 2


def test_empty_plugin_name_rejected():
    assert asynciodat.set_plugin("", object()) is False


def test_del_missing_plugin_returns_false():
    assert asynciodat.del_plugin("never-set") is False


def test_clear_plugins():
    asynciodat.set_plugin("a", 1)
    asynciodat.set_plugin("b", 2)
    asynciodat.clear_plugins()
    assert list(asynciodat.plugin_names()) == []


def test_many_plugins_roundtrip():
    for i in range(100):
        assert asynciodat.set_plugin(f"p{i}", i) is True
    names = list(asynciodat.plugin_names())
    assert len(names) == 100
    assert asynciodat.get_plugin("p57") == 57
    asynciodat.clear_plugins()
    assert list(asynciodat.plugin_names()) == []


def test_plugin_async_method_mutates_state_over_polls():
    """A stored plugin's async method runs to completion and mutates the
    plugin's state — the cross-frame plugin use case, headless."""
    class Counter:
        def __init__(self):
            self.value = 0

        async def add(self, amount):
            await asyncio.sleep(0.02)
            self.value += amount

    counter = Counter()
    asynciodat.set_plugin("counter", counter)
    asynciodat.add_task(counter.add(5))
    assert pump_until(lambda: counter.value == 5)
    assert asynciodat.get_plugin("counter").value == 5
