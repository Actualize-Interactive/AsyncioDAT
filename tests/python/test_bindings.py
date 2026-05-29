"""pytest suite for the AsyncioDAT bindings, via the `asynciodat` test extension.

The extension creates one operator instance at import time (which initializes
the asyncio event loop), so the binding functions operate on a live instance —
no TouchDesigner required.
"""

import asyncio

import asynciodat


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


def test_plugin_roundtrip():
    asynciodat.clear_plugins()
    assert asynciodat.has_plugin("x") is False

    assert asynciodat.set_plugin("x", {"a": 1}) is True
    assert asynciodat.has_plugin("x") is True
    assert asynciodat.get_plugin("x") == {"a": 1}
    assert "x" in list(asynciodat.plugin_names())

    assert asynciodat.del_plugin("x") is True
    assert asynciodat.has_plugin("x") is False
    assert asynciodat.get_plugin("x") is None


def test_set_plugin_overwrites():
    asynciodat.clear_plugins()
    asynciodat.set_plugin("k", 1)
    asynciodat.set_plugin("k", 2)
    assert asynciodat.get_plugin("k") == 2


def test_empty_plugin_name_rejected():
    assert asynciodat.set_plugin("", object()) is False


def test_clear_plugins():
    asynciodat.clear_plugins()
    asynciodat.set_plugin("a", 1)
    asynciodat.set_plugin("b", 2)
    asynciodat.clear_plugins()
    assert list(asynciodat.plugin_names()) == []


def test_add_task_runs_when_polled():
    result = {}

    async def job():
        await asyncio.sleep(0)
        result["value"] = 21 + 21

    assert asynciodat.add_task(job()) is True
    for _ in range(10):
        asynciodat.poll_event_loop()
    assert result.get("value") == 42


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
    for _ in range(10):
        asynciodat.poll_event_loop()
    assert task.done()
