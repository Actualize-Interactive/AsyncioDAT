"""In-TouchDesigner integration test runner for AsyncioDAT (Tier 2).

This runs *inside* TouchDesigner. It is invoked from a one-time bootstrap
Execute DAT (see TESTING.md), runs the test suites, writes a `results.json`
sentinel, and quits TouchDesigner so a host script (run_td_tests.ps1) can gate
on the outcome.

Flow:
    onStart  -> td_test_runner.start()   # run synchronous tests, schedule finish
    (frames) -> td_test_runner.finish()  # check async results, write json, quit

Environment variables (set by run_td_tests.ps1):
    ASYNCIODAT_RESULTS        path to write results.json (default: <project>/results.json)
    ASYNCIODAT_OP             name of the AsyncioDAT operator (default: Asyncio1)
    ASYNCIODAT_SETTLE_FRAMES  frames to wait for async tasks (default: 240)
"""

import os
import json
import asyncio
import builtins
import traceback

OP_NAME = os.environ.get("ASYNCIODAT_OP", "Asyncio1")
SETTLE_FRAMES = int(os.environ.get("ASYNCIODAT_SETTLE_FRAMES", "240"))

_state = {"results": [], "async_done": None}


def _results_path():
    # project.folder is the directory containing the open .toe (the test/ dir).
    default = os.path.join(project.folder, "results.json")  # noqa: F821 (TD global)
    return os.environ.get("ASYNCIODAT_RESULTS", default)


def _record(name, passed, detail=""):
    _state["results"].append(
        {"name": name, "passed": bool(passed), "detail": str(detail)}
    )
    print(f"[td-test] {'PASS' if passed else 'FAIL'}  {name}  {detail}".rstrip())


def _add_test_paths():
    import sys
    test_dir = project.folder  # noqa: F821
    for sub in ("", "test_scripts"):
        p = os.path.join(test_dir, sub) if sub else test_dir
        if os.path.isdir(p) and p not in sys.path:
            sys.path.insert(0, p)


def start():
    """Entry point called from the bootstrap Execute DAT's onStart()."""
    print("[td-test] start()")
    try:
        _add_test_paths()

        adat = op(OP_NAME)  # noqa: F821 (TD global)
        _record("operator_exists", adat is not None, OP_NAME)
        if adat is None:
            finish()
            return

        _record("is_running", bool(adat.is_running()))

        # --- Plugin suite (synchronous) -----------------------------------
        try:
            builtins.ASYNCIODAT_TEST_IMPORT = True
            import importlib
            import test_plugins
            importlib.reload(test_plugins)
            ok = test_plugins.run_all_tests()
            _record("plugin_suite", ok)
        except Exception:
            _record("plugin_suite", False, traceback.format_exc())
        finally:
            builtins.ASYNCIODAT_TEST_IMPORT = False

        # --- Async smoke test (verified in finish() after some frames) -----
        flag = {"completed": False, "value": None}

        async def _smoke():
            await asyncio.sleep(0.25)
            flag["value"] = 21 + 21
            flag["completed"] = True

        try:
            adat.add_task(_smoke())
            _state["async_done"] = flag
        except Exception:
            _record("async_task_scheduled", False, traceback.format_exc())

    except Exception:
        _record("runner_start", False, traceback.format_exc())

    # Schedule finish() after the loop has had frames to run the coroutine.
    run("import td_test_runner; td_test_runner.finish()",  # noqa: F821 (TD global)
        delayFrames=SETTLE_FRAMES)


def finish():
    """Check async results, write results.json, and quit TouchDesigner."""
    print("[td-test] finish()")
    try:
        flag = _state.get("async_done")
        if flag is not None:
            _record(
                "async_task_completed",
                flag["completed"] and flag["value"] == 42,
                f"value={flag['value']}",
            )

        results = _state["results"]
        summary = {
            "results": results,
            "passed": sum(1 for r in results if r["passed"]),
            "failed": sum(1 for r in results if not r["passed"]),
            "success": len(results) > 0 and all(r["passed"] for r in results),
        }
        path = _results_path()
        with open(path, "w") as f:
            json.dump(summary, f, indent=2)
        print(f"[td-test] wrote {path}: "
              f"{summary['passed']} passed, {summary['failed']} failed")
    except Exception:
        print("[td-test] finish() error:\n" + traceback.format_exc())
    finally:
        try:
            project.quit(force=True)  # noqa: F821 (TD global)
        except Exception:
            print("[td-test] project.quit failed:\n" + traceback.format_exc())
