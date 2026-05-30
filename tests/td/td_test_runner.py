"""In-TouchDesigner integration test driver for AsyncioDAT.

Loaded as a module in TouchDesigner (a DAT under /local/modules, synced to this
file) and invoked from a bootstrap Execute DAT's onStart, which passes the
AsyncioDAT operator in — the only place that needs to know where it lives:

    def onStart():
        import td_test_runner
        td_test_runner.start(op('Asyncio1'))
        return

start() schedules the integration suite (asyncio_test.run_all_tests) on the
operator's event loop and returns immediately. The suite is a coroutine that
awaits across real frames, advanced by the operator's per-frame polling; when it
finishes, this module writes the results.json sentinel. TouchDesigner is left
running and the host script (run_td_tests.ps1 / run_td_tests.sh) detects the
sentinel and terminates TouchDesigner.

Environment variables (set by the host script):
    ASYNCIODAT_RESULTS  path to write results.json (default: <project>/results.json)
"""

import os
import json

import asyncio_test


def _results_path():
    default = os.path.join(project.folder, "results.json")  # noqa: F821 (TD global)
    return os.environ.get("ASYNCIODAT_RESULTS", default)


def start(asyncio_dat):
    """Run the synchronous checks, then schedule the async suite on the loop."""
    print("[td-test] start()")
    # The async suite advances by the operator polling its loop each frame, so
    # Auto Poll must be on.
    asyncio_dat.par.Autopoll = 1
    sync_results = asyncio_test.run_sync_checks(asyncio_dat)
    asyncio_dat.add_task(_run(asyncio_dat, sync_results))


async def _run(asyncio_dat, sync_results):
    async_results = await asyncio_test.run_all_tests(asyncio_dat)
    _write_results(sync_results + async_results)


def _write_results(results):
    summary = {
        "results": results,
        "passed": sum(1 for x in results if x["passed"]),
        "failed": sum(1 for x in results if not x["passed"]),
        "success": len(results) > 0 and all(x["passed"] for x in results),
    }
    path = _results_path()
    with open(path, "w") as f:
        json.dump(summary, f, indent=2)
    print(f"[td-test] wrote {path}: "
          f"{summary['passed']} passed, {summary['failed']} failed")
