# AsyncioDAT Testing Instructions

There are two layers of testing:

1. **Build** the operator (see [README.md](README.md) → Building from Source) and
   confirm it loads in TouchDesigner.
2. **Functional tests** — the Python scripts under `test/test_scripts/` and the
   gRPC suite in `test/`, run inside TouchDesigner.

> Note: TouchDesigner cannot run in cloud CI (it needs a license and a GPU), so
> the GitHub Actions workflows only build the operator. Functional tests run
> against a local TouchDesigner install.

## Automated integration test (local)

`run_td_tests.ps1` is a local pre-release gate. It launches TouchDesigner with
`test/test.toe`, runs the test suites *inside* TouchDesigner, writes a
`results.json` sentinel, quits TouchDesigner, then parses the results and exits
non-zero if anything failed.

```powershell
# Build the operator and run the full integration test:
.\run_td_tests.ps1 -Build

# Options:
.\run_td_tests.ps1 -OpName Asyncio1 -TimeoutSec 180 -SettleFrames 240
.\run_td_tests.ps1 -Grpc          # also exercise the gRPC server from outside TD
```

The in-TD logic lives in `test/td_tests/td_test_runner.py` (`start()` runs the
synchronous suites and schedules `finish()`, which verifies the async smoke test,
writes `results.json`, and calls `project.quit()`).

### One-time wiring (bootstrap Execute DAT)

TouchDesigner runs project code via DAT callbacks, so `test.toe` needs a small
Execute DAT that kicks off the runner on start. This is a one-time setup:

1. In `test.toe`, add an **Execute DAT** at the root.
2. Enable its **Start** flag (the `onStart` callback).
3. Paste:

   ```python
   def onStart():
       import sys, os
       td_tests = os.path.join(project.folder, 'td_tests')
       if td_tests not in sys.path:
           sys.path.insert(0, td_tests)
       import td_test_runner
       td_test_runner.start()
       return
   ```

4. **Save** `test.toe`.

> The bootstrap intentionally lives in the `.toe` (a binary file) rather than in
> the repo, so it only needs to be wired once. If you prefer to keep `.toe`
> files untouched, `toeexpand` / `toecollapse` (shipped with TouchDesigner) can
> inject the Execute DAT without opening the GUI — a possible future enhancement
> to make this fully turnkey.

## Setup

1. **Build the Project**:
   ```powershell
   .\build.ps1
   ```

2. **Open TouchDesigner**:
   - Open `test/test.toe` in TouchDesigner
   - The AsyncioDAT.dll should be automatically loaded from the Plugins directory

3. **Create an AsyncioDAT Operator**:
   - In the TouchDesigner network, create a new DAT operator
   - Type "Asyncio" in the operator palette to find AsyncioDAT
   - Name the operator `Asyncio1` (or update the test scripts accordingly)

## Testing the Implementation

### Method 1: Using the Test Scripts

1. **Load the comprehensive async test**:
   - Create a Text DAT operator
   - Load the content from `test/test_scripts/asyncio_test.py`
   - Set the Text DAT to Python mode and run it (it calls `run_all_tests()` on load)

2. **Load the plugin test**:
   - Create another Text DAT operator
   - Load the content from `test/test_scripts/test_plugins.py`
   - Run it (it calls `run_all_tests()` on load)

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

The AsyncioDAT operator provides real-time statistics:
- Execute count (frames processed)
- Event loop status
- Auto-processing state
- Available methods

Monitor these values to ensure proper operation and identify potential issues.
