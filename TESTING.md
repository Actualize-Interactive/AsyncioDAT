# AsyncioDAT Testing Instructions

Tests live under `tests/`:

- `tests/cpp/` — Catch2 unit tests (no TouchDesigner needed).
- `tests/python/` — pytest against a compiled test extension (no TouchDesigner needed).
- `tests/td/` — the TouchDesigner project, the in-network test scripts, and the
  local integration harness (`run_td_tests.ps1`).

> Note: TouchDesigner cannot run in cloud CI (it needs a license and a GPU), so
> the GitHub Actions workflows only build the operator and run the `tests/cpp`
> and `tests/python` suites. The `tests/td` integration test runs against a
> local TouchDesigner install.

## Unit tests (no TouchDesigner)

The `tests/cpp` (Catch2) and `tests/python` (pytest) suites build and run
without TouchDesigner, and run in CI on Windows and macOS.

```powershell
# Configure + build + run ALL unit tests in one command:
cmake --workflow --preset dev

# Re-run just the unit tests after a change:
ctest --preset dev
```

CMake auto-detects the uv Python 3.11 (no paths to pass), and the pytest suite
is run through `uv run`, so pytest is fetched automatically — nothing to install
first. (In environments without uv, install `tests/python/requirements.txt` into
the interpreter and ctest will call `pytest` there instead.)

- `tests/cpp` covers the pure config/TOML parsing.
- `tests/python` builds a small CPython extension (`asynciodat`) that compiles
  the real operator sources against a fake `OP_Context`, so the asyncio /
  plugin / config logic is exercised directly. Once the extension is built you
  can also run pytest on its own:

  ```powershell
  uv run --with pytest pytest tests/python
  ```

## Automated integration test (local)

`run_td_tests.ps1` is a local pre-release gate. Run the one script and wait for
pass/fail — it does everything for you: compiles the operator, copies it into
`tests/td/Plugins/`, launches TouchDesigner with `tests/td/test.toe`, runs the
test suites *inside* TouchDesigner, writes a `results.json` sentinel, then
parses the results, terminates TouchDesigner, and exits non-zero if anything
failed.

```powershell
# Build + copy the plugin, run the full integration test, report pass/fail:
.\run_td_tests.ps1

# Options:
.\run_td_tests.ps1 -NoBuild      # reuse the already-built plugin (skip compiling)
.\run_td_tests.ps1 -OpName Asyncio1 -TimeoutSec 180
```

Two modules implement it. `td_test_runner.py` schedules the suite on the
operator's loop (`start(asyncio_dat)`) and writes `results.json` when it
finishes. `asyncio_test.py` *is* the suite: a coroutine that `await`s across
real frames — the correct way to wait N frames in TouchDesigner — and asserts
deferred timing, completion order, concurrency, exception isolation,
cancellation, task results, plugin async behaviour, and parameter mutation over
frames. The operator is passed in, so neither module needs to know where it
lives. TouchDesigner is left running; the host script terminates it once the
sentinel appears.

### One-time wiring (modules + bootstrap Execute DAT)

The test modules are loaded the TouchDesigner way — as DATs under
`/local/modules`, each synced to its file on disk so the repo stays the source
of truth:

| Module DAT (`/local/modules/…`) | Synced to file |
| --- | --- |
| `td_test_runner` | `tests/td/td_test_runner.py` |
| `asyncio_test`   | `tests/td/asyncio_test.py` |

(In each DAT, set the **File** parameter to the path above and use **Sync to
File** so TouchDesigner imports them by name.)

Then add an **Execute DAT** (anywhere) that starts the runner and passes it the
AsyncioDAT operator — this is the only place that needs to know where the
operator is:

1. Enable the Execute DAT's **Start** flag (the `onStart` callback).
2. Paste:

   ```python
   def onStart():
       import td_test_runner
       td_test_runner.start(op('Asyncio1'))   # point at your AsyncioDAT
       return
   ```

3. **Save** `test.toe`.

## Setup

1. **Build the Project**:
   ```powershell
   .\build.ps1
   ```

2. **Open TouchDesigner**:
   - Open `tests/td/test.toe` in TouchDesigner
   - The plugin is loaded from `tests/td/Plugins/` — TouchDesigner loads Custom
     Operators from a `Plugins/` folder beside the `.toe`, and there is no way to
     point it at an arbitrary build directory. `build.ps1` copies the freshly
     built operator there for you.
   - **First load after a (re)build:** TouchDesigner shows a modal asking you to
     approve/trust the newly built Custom Operator before it will load it. Click
     to approve. This is interactive, so the very first integration-test run
     after a rebuild may need a manual approval click.

3. **Create an AsyncioDAT Operator**:
   - In the TouchDesigner network, create a new DAT operator
   - Type "Asyncio" in the operator palette to find AsyncioDAT
   - Name the operator `Asyncio1` (or update the test scripts accordingly)

## Testing the Implementation

### Method 1: Run the suite manually

`asyncio_test.run_all_tests(asyncio_dat)` is a coroutine, so schedule it on the
operator and watch the textport for the `[td-test]` PASS/FAIL lines (replace
`Asyncio1` with your operator's name):

```python
import asyncio_test                          # DAT under /local/modules
adat = op('Asyncio1')
adat.add_task(asyncio_test.run_all_tests(adat))
```

Or just run `.\run_td_tests.ps1`, which also captures the results to a file.

### Method 2: Manual Testing

1. **Basic functionality test**:
   ```python
   # In TouchDesigner Python console or textDAT:
   
   import asyncio
   
   # Get the AsyncioDAT operator
   asyncio_op = op('Asyncio1')
   
   # Check status
   print(f"Is running: {asyncio_op.is_running()}")
   
   # Create a simple async function
   async def test_task():
       print("Starting async task...")
       await asyncio.sleep(2.0)
       print("Async task completed!")
   
   # Add the task to the event loop
   asyncio_op.add_task(test_task())
   ```

2. **Concurrent tasks test**:
   ```python
   async def worker(worker_id):
       for i in range(3):
           print(f"Worker {worker_id}: step {i}")
           await asyncio.sleep(1.0)
       print(f"Worker {worker_id} finished!")
   
   # Start multiple workers
   for i in range(3):
       asyncio_op.add_task(worker(i))
   ```

### Method 3: Interactive Testing

1. **Enable Auto Poll**: Make sure the "Auto Poll" parameter is enabled on the AsyncioDAT operator

2. **Use the Reset button**: If something goes wrong, use the "Reset" pulse parameter to reinitialize the event loop

3. **Monitor the output**: The AsyncioDAT operator displays status information in its output

## Expected Behavior

### Successful Operation
- AsyncioDAT status shows "Asyncio Initialized: Yes"
- "Event Loop Running: Yes"
- Console output shows async tasks executing concurrently
- TouchDesigner remains responsive and doesn't freeze
- Frame rate is unaffected by async operations

### Common Issues and Solutions

1. **"Asyncio not initialized" error**:
   - Check that the AsyncioDAT operator was created successfully
   - Try pulsing the "Reset" parameter
   - Verify the plugin DLL is loaded correctly

2. **Tasks not executing**:
   - Ensure "Auto Poll" is enabled
   - Check that the operator is cooking every frame
   - Verify the async functions are properly defined with `async def`

3. **Performance issues**:
   - Monitor the number of concurrent tasks
   - Avoid blocking operations in async functions
   - Use `await asyncio.sleep(0)` to yield control in long-running loops

## Verification Steps

1. **Plugin Loading**: Check TouchDesigner's console for any DLL loading errors
2. **Operator Creation**: Verify the AsyncioDAT operator appears in the palette
3. **Initialization**: Check the operator's output for initialization status
4. **Task Execution**: Run test scripts and verify console output
5. **Performance**: Confirm TouchDesigner remains responsive during async operations

## Advanced Testing

### Network Requests (requires aiohttp)
```python
import aiohttp

async def fetch_url(url):
    async with aiohttp.ClientSession() as session:
        async with session.get(url) as response:
            text = await response.text()
            print(f"Fetched {len(text)} characters from {url}")
            return text

# Test with a delay endpoint
asyncio_op.add_task(fetch_url('https://httpbin.org/delay/2'))
```

### File I/O Operations
```python
import aiofiles

async def read_file_async(filename):
    try:
        async with aiofiles.open(filename, 'r') as f:
            content = await f.read()
            print(f"Read {len(content)} characters from {filename}")
            return content
    except Exception as e:
        print(f"Error reading file: {e}")
        return None

asyncio_op.add_task(read_file_async('test.txt'))
```

### Periodic Tasks
```python
async def heartbeat():
    while True:
        print(f"Heartbeat: {time.time()}")
        await asyncio.sleep(5.0)

# This will run indefinitely
asyncio_op.add_task(heartbeat())
```

## Debugging Tips

1. **Enable verbose output**: Add print statements to async functions for debugging
2. **Check operator status**: Monitor the AsyncioDAT operator's text output
3. **Use try/except**: Wrap risky operations in try/except blocks
4. **Monitor resources**: Watch for memory leaks with long-running tasks
5. **Test incremental**: Start with simple tasks before moving to complex operations

## Performance Monitoring

Attach an Info CHOP to the AsyncioDAT operator for real-time statistics: `event_loop_active`, `event_loop_auto_poll`, `event_loop_poll_count`, and `event_loop_poll_duration` (milliseconds spent in the last poll). Monitor these values to ensure proper operation and identify potential issues — a poll duration that climbs with the number of tasks is the signal that something in a coroutine is blocking rather than awaiting.
