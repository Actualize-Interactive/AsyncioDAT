"""Integration tests for AsyncioDAT, run inside TouchDesigner.

These are integration tests, not unit tests. The whole point of AsyncioDAT is
that a coroutine scheduled now has its effects happen many frames later, driven
by the operator polling its event loop once per frame. So the bulk of the suite
is itself a coroutine: it `await`s across real frames (that is the correct way
to wait N frames in TouchDesigner) and asserts that scheduled work actually
happens, in the right order, concurrently, and that failures don't take down the
loop.

Two entry points, both used by td_test_runner:

- run_sync_checks(asyncio_dat): synchronous checks (properties, get_event_loop,
  and the blocking run_coroutine, which can't be called from inside the running
  loop). Run before the async suite is scheduled.
- run_all_tests(asyncio_dat): the async, frame-spanning suite. Awaited by the
  driver, which writes results.json when it returns.

The operator must be polling its loop every frame (Auto Poll) for the awaited
work to advance — td_test_runner ensures that before scheduling the suite.
"""

import time
import asyncio
import traceback


class Results:
    """Collects pass/fail records and echoes them to the textport."""

    def __init__(self):
        self.items = []

    def record(self, name, passed, detail=""):
        self.items.append({"name": name, "passed": bool(passed), "detail": str(detail)})
        print(f"[td-test] {'PASS' if passed else 'FAIL'}  {name}  {detail}".rstrip())


def run_sync_checks(asyncio_dat):
    """Synchronous checks that don't need frames. Returns a list of results."""
    r = Results()

    r.record("is_running", asyncio_dat.is_running())
    r.record("loop_running", asyncio_dat.loop_running)
    r.record("asyncio_initialized", asyncio_dat.asyncio_initialized)

    loop = asyncio_dat.get_event_loop()
    r.record("get_event_loop", isinstance(loop, asyncio.AbstractEventLoop),
             f"type={type(loop).__name__}")

    # run_coroutine is blocking (run_until_complete) and is valid here because we
    # are in synchronous context, not inside the running loop.
    async def compute():
        await asyncio.sleep(0)
        return 7
    r.record("run_coroutine", asyncio_dat.run_coroutine(compute()) == 7)

    return r.items


# --- Async, frame-spanning tests ------------------------------------------

async def deferred_execution(adat, r):
    """A scheduled coroutine's effect happens later, not on the scheduling frame."""
    state = {"ran": False}

    async def action():
        await asyncio.sleep(1.0)
        state["ran"] = True

    adat.add_task(action())
    r.record("deferred_not_immediate", state["ran"] is False)
    await asyncio.sleep(1.5)
    r.record("deferred_ran_later", state["ran"] is True)


async def completion_order_by_delay(adat, r):
    """Tasks with different delays complete in delay order, not scheduling order."""
    order = []

    async def task(name, delay):
        await asyncio.sleep(delay)
        order.append(name)

    adat.add_task(task("c", 3.0))
    adat.add_task(task("a", 1.0))
    adat.add_task(task("b", 2.0))
    await asyncio.sleep(3.5)
    r.record("completion_order_by_delay", order == ["a", "b", "c"], f"order={order}")


async def fifo_order_same_delay(adat, r):
    """Tasks with the same (zero) delay complete in the order they were scheduled."""
    order = []

    async def task(name):
        await asyncio.sleep(0)
        order.append(name)

    for name in ("first", "second", "third"):
        adat.add_task(task(name))
    await asyncio.sleep(0.5)
    r.record("fifo_order_same_delay", order == ["first", "second", "third"], f"order={order}")


async def periodic_ticks(adat, r):
    """A looping coroutine yields repeatedly over frames; ticks land in order.

    Mirrors the original periodic_counter(0.5, 8): a task that lives for several
    seconds, ticking once every half second across ~240 frames at 60fps.
    """
    ticks = []

    async def ticker(count, interval):
        for i in range(count):
            await asyncio.sleep(interval)
            ticks.append(i)

    adat.add_task(ticker(8, 0.5))
    await asyncio.sleep(4.5)
    r.record("periodic_ticks_in_order", ticks == list(range(8)), f"ticks={ticks}")


async def concurrent_overlap(adat, r):
    """N tasks each sleeping d run concurrently (~d total), not serially (~N*d)."""
    done = []

    async def worker(i):
        await asyncio.sleep(1.0)
        done.append(i)

    start = time.monotonic()
    for i in range(5):
        adat.add_task(worker(i))
    await asyncio.sleep(1.5)
    elapsed = time.monotonic() - start
    r.record("concurrent_all_complete", len(done) == 5, f"done={len(done)}/5")
    r.record("concurrent_overlapped", elapsed < 2.0, f"elapsed={elapsed:.2f}s (serial ~5.0s)")


async def stress_many_tasks(adat, r):
    """A large batch of concurrent tasks all complete, and not serially."""
    done = []
    count = 20

    async def worker(i):
        await asyncio.sleep(1.0)
        done.append(i)

    start = time.monotonic()
    for i in range(count):
        adat.add_task(worker(i))
    await asyncio.sleep(1.5)
    elapsed = time.monotonic() - start
    r.record("stress_all_complete", len(done) == count, f"done={len(done)}/{count}")
    r.record("stress_not_serial", elapsed < 3.0, f"elapsed={elapsed:.2f}s (serial ~20.0s)")


async def exception_isolated(adat, r):
    """A task that raises doesn't stop the loop or other scheduled tasks."""
    survived = {"ran": False}

    async def boom():
        await asyncio.sleep(0.5)
        raise ValueError("intentional failure")

    async def survivor():
        await asyncio.sleep(1.0)
        survived["ran"] = True

    adat.add_task(boom())
    adat.add_task(survivor())
    await asyncio.sleep(1.5)
    r.record("exception_does_not_break_loop", survived["ran"] is True)


async def create_task_awaitable(adat, r):
    """create_task returns a Task that yields its result when awaited."""
    async def compute():
        await asyncio.sleep(1.0)
        return 99

    result = await adat.create_task(compute())
    r.record("create_task_result", result == 99, f"result={result}")


async def cancellation(adat, r):
    """A cancelled task is marked cancelled and its later effect never happens."""
    state = {"ran": False}

    async def action():
        await asyncio.sleep(2.0)
        state["ran"] = True

    task = adat.create_task(action())
    await asyncio.sleep(0.5)
    task.cancel()
    await asyncio.sleep(2.0)
    r.record("cancelled_task_did_not_run", state["ran"] is False)
    r.record("cancelled_task_is_cancelled", task.cancelled())


async def parameter_mutation_over_frames(adat, r):
    """Reset a parameter to its default, then drive it through a sequence of
    values at staggered delays.

    This is the real use case (the original test_set_pars): reset pars to their
    defaults, then a single scheduled coroutine sets them one after another with
    a delay between each, so the network sees the parameter change at distinct
    times across many frames — not all at once.
    """
    par = adat.par.Maxstatusrows
    default = int(par.default)

    # Reset to default first, as the original did before driving values.
    par.val = default

    offset = 1.0
    schedule = [11, 22, 33]  # values set at +1s, +2s, +3s

    async def drive():
        for value in schedule:
            await asyncio.sleep(offset)
            par.val = value

    adat.add_task(drive())

    # Before the first set fires, the par is still at its default.
    await asyncio.sleep(0.5)
    r.record("par_reset_to_default", int(par.eval()) == default, f"val={par.eval()}")

    # Each scheduled set lands ~1s apart; sample just after each.
    await asyncio.sleep(offset)
    r.record("par_first_value", int(par.eval()) == 11, f"val={par.eval()}")
    await asyncio.sleep(offset)
    r.record("par_second_value", int(par.eval()) == 22, f"val={par.eval()}")
    await asyncio.sleep(offset)
    r.record("par_third_value", int(par.eval()) == 33, f"val={par.eval()}")

    par.val = default  # restore


async def plugin_async_method(adat, r):
    """A stored plugin's async method runs to completion and mutates plugin state."""
    class Counter:
        def __init__(self):
            self.value = 0

        async def add(self, amount):
            await asyncio.sleep(1.0)
            self.value += amount

    counter = Counter()
    adat.set_plugin("counter", counter)
    r.record("plugin_same_object", adat.get_plugin("counter") is counter)

    adat.add_task(counter.add(5))
    await asyncio.sleep(1.5)
    r.record("plugin_async_method_ran", counter.value == 5, f"value={counter.value}")

    adat.del_plugin("counter")
    r.record("plugin_deleted", not adat.has_plugin("counter"))


TESTS = [
    deferred_execution,
    completion_order_by_delay,
    fifo_order_same_delay,
    periodic_ticks,
    concurrent_overlap,
    stress_many_tasks,
    exception_isolated,
    create_task_awaitable,
    cancellation,
    parameter_mutation_over_frames,
    plugin_async_method,
]


async def run_all_tests(asyncio_dat):
    """Run every frame-spanning test and return a list of result dicts.

    Each test reports its own pass/fail via the Results recorder. If a test
    raises unexpectedly it is recorded as a failure so the rest still run.
    """
    r = Results()
    for test in TESTS:
        try:
            await test(asyncio_dat, r)
        except Exception:
            r.record(test.__name__, False, traceback.format_exc())
    return r.items
