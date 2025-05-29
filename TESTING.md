# AsyncioDAT Testing Instructions

## Testing the Build System

### Workflow Validation

Before testing the AsyncioDAT functionality, you can validate that the CI and Release workflows are properly configured:

```bash
python validate_workflows.py
```

This script checks:
- YAML structure validity
- Platform consistency between CI and Release workflows  
- Artifact path consistency
- Build tool version consistency
- Expected artifact presence (AsyncioDAT.dll, AsyncioDAT.dylib)

The validation ensures that both workflows will produce the correct artifacts for Windows (.dll) and macOS (.dylib).

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
   - Name the operator `asynciodat1` (or update the test scripts accordingly)

## Testing the Implementation

### Method 1: Using the Test Scripts

1. **Load the simple example**:
   - Create a textDAT operator
   - Load the content from `test/simple_example.py`
   - Set the textDAT to Python mode
   - Execute the script

2. **Load the comprehensive test**:
   - Create another textDAT operator
   - Load the content from `test/asyncio_test.py`
   - Set the textDAT to Python mode
   - Execute the script and run `run_all_tests()`

### Method 2: Manual Testing

1. **Basic functionality test**:
   ```python
   # In TouchDesigner Python console or textDAT:
   
   import asyncio
   
   # Get the AsyncioDAT operator
   asyncio_op = op('asynciodat1')
   
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

1. **Enable Auto Process Events**: Make sure the "Auto Process Events" parameter is enabled on the AsyncioDAT operator

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
   - Ensure "Auto Process Events" is enabled
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
