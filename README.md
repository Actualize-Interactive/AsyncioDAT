# AsyncioDAT - Asyncio Event Loop for TouchDesigner

AsyncioDAT is a C++ TouchDesigner operator that provides a managed asyncio event loop for Python scripting within TouchDesigner. It enables truly asynchronous programming without blocking the main TouchDesigner thread.

## Features

- **Automatic Event Loop Management**: Initializes, manages, and tears down an asyncio event loop automatically
- **Frame-Synchronized Processing**: Processes asyncio events once per TouchDesigner frame for smooth integration
- **Non-Blocking Operations**: Enables async I/O, network requests, and other long-running operations without freezing TouchDesigner
- **Easy Python Integration**: Simple Python API for adding coroutines and managing async tasks
- **Comprehensive Error Handling**: Robust error handling and recovery mechanisms

## Installation

1. Build the project using the provided PowerShell script:
   ```powershell
   .\build.ps1
   ```

2. The DLL will be automatically copied to the `test/Plugins/` directory

3. Open the test TouchDesigner file: `test/test.toe`

## Configuration

### TOML Configuration (Optional)

AsyncioDAT supports loading configuration from a `config.toml` file in the working directory. This allows you to specify custom Python paths and callback locations without modifying code.

Create a `config.toml` file with the following structure:

```toml
[main]
paths = [
    "path/to/your/modules",
    "another/path/with spaces",
    ".venv/Lib/site-packages"
]

[asyncio]
callback_module_path = "path/to/your/callback.py"
```

**Configuration sections:**

- `[main]` section:
  - `paths`: Array of directory paths to prepend to `sys.path` for Python module discovery
  
- `[asyncio]` section:
  - `callback_module_path`: Path to the Python file containing the `on_create` callback function

**Backward Compatibility:**

If no `config.toml` is found, AsyncioDAT falls back to the original behavior:
- Loads paths from `prepend_to_path.txt`
- Uses `on_asyncio_create.py` as the callback file

## Usage

### Basic Setup

1. Add an AsyncioDAT operator to your TouchDesigner network
2. Ensure "Auto Process Events" parameter is enabled (default)
3. The operator will automatically initialize an asyncio event loop

### Python API

The AsyncioDAT operator exposes the following methods:

#### Core Methods

```python
# Get reference to the operator
asyncio_op = op('asynciodat1')

# Initialize asyncio (called automatically)
asyncio_op.initialize_asyncio()

# Shutdown asyncio
asyncio_op.shutdown_asyncio()

# Process events manually (if auto-processing is disabled)
asyncio_op.process_events()

# Check if asyncio is running
is_running = asyncio_op.is_running()
```

#### Task Management

```python
# Add a coroutine as a task to the event loop
async def my_async_function():
    await asyncio.sleep(1)
    print("Async task completed!")

coro = my_async_function()
success = asyncio_op.add_task(coro)

# Create a task object (returns the task for manipulation)
task = asyncio_op.create_task(coro)

# Run a coroutine until completion (blocking)
result = asyncio_op.run_coroutine(coro)
```

#### Event Loop Access

```python
# Get the current event loop object
loop = asyncio_op.get_event_loop()

# Use the loop directly for advanced operations
if loop:
    loop.call_later(5.0, lambda: print("Called after 5 seconds"))
```

### Example Usage

#### Simple Async Task

```python
import asyncio

async def simple_task():
    print("Starting async task...")
    await asyncio.sleep(2.0)
    print("Async task completed!")
    return "Task result"

# Add to the managed event loop
op('asynciodat1').add_task(simple_task())
```

#### Periodic Task

```python
async def periodic_task():
    count = 0
    while count < 10:
        print(f"Periodic task tick: {count}")
        count += 1
        await asyncio.sleep(1.0)

op('asynciodat1').add_task(periodic_task())
```

#### HTTP Requests with aiohttp

```python
import aiohttp
import asyncio

async def fetch_data(url):
    async with aiohttp.ClientSession() as session:
        async with session.get(url) as response:
            data = await response.text()
            print(f"Received {len(data)} characters from {url}")
            return data

# Fetch data asynchronously without blocking TouchDesigner
url = "https://httpbin.org/delay/2"
op('asynciodat1').add_task(fetch_data(url))
```

#### Concurrent Tasks

```python
async def worker(worker_id, duration):
    print(f"Worker {worker_id} starting...")
    await asyncio.sleep(duration)
    print(f"Worker {worker_id} finished!")
    return f"Result from worker {worker_id}"

# Start multiple workers concurrently
for i in range(5):
    op('asynciodat1').add_task(worker(i, random.uniform(1, 3)))
```

### Operator Parameters

- **Auto Process Events**: Toggle automatic event processing every frame (default: enabled)
- **Reset**: Pulse to shutdown and reinitialize the asyncio event loop

### Properties

- `loop_running`: Read-only boolean indicating if the event loop is active
- `asyncio_initialized`: Read-only boolean indicating if asyncio is properly initialized

## Technical Details

### Event Loop Processing

The AsyncioDAT operator uses the `loop.run_until_complete(asyncio.sleep(0))` pattern to process pending events without blocking. This approach:

- Allows all ready tasks to execute
- Processes I/O completions
- Runs scheduled callbacks
- Yields control back to TouchDesigner quickly

### Threading Model

- The asyncio event loop runs on TouchDesigner's main thread
- Event processing is synchronized with TouchDesigner's frame rate
- Async operations don't block the UI or rendering
- Python Global Interpreter Lock (GIL) is properly managed

### Error Handling

- Task exceptions are caught and logged to the console
- Event loop errors trigger automatic reinitialization
- Robust cleanup on operator destruction
- Graceful handling of Python environment changes

## Best Practices

### 1. Use for I/O-Bound Operations

AsyncioDAT is ideal for:
- Network requests (HTTP, WebSocket, etc.)
- File I/O operations
- Database queries
- Inter-process communication
- Waiting for external events

### 2. Avoid CPU-Intensive Tasks

For CPU-intensive work, consider:
- Using `asyncio.to_thread()` for thread execution
- Breaking work into smaller chunks with `await asyncio.sleep(0)`
- Using TouchDesigner's built-in multithreading where appropriate

### 3. Resource Management

```python
# Good: Use context managers for resources
async def good_example():
    async with aiohttp.ClientSession() as session:
        async with session.get(url) as response:
            return await response.text()

# Good: Clean up resources properly
async def cleanup_example():
    try:
        # Some async operation
        await some_operation()
    finally:
        # Cleanup code
        await cleanup_resources()
```

### 4. Error Handling

```python
async def robust_task():
    try:
        result = await risky_operation()
        return result
    except Exception as e:
        print(f"Task failed: {e}")
        return None
```

## Troubleshooting

### Common Issues

1. **"Asyncio not initialized" Error**
   - Ensure the AsyncioDAT operator is properly created
   - Check that initialization completed successfully
   - Try using the Reset pulse parameter

2. **Tasks Not Executing**
   - Verify "Auto Process Events" is enabled
   - Check the operator's status output for error messages
   - Ensure the operator is cooking every frame

3. **Performance Issues**
   - Avoid blocking operations in async functions
   - Use appropriate await points with `asyncio.sleep(0)`
   - Monitor the number of concurrent tasks

4. **Memory Leaks**
   - Ensure proper cleanup of resources
   - Use context managers for file/network operations
   - Check for circular references in task closures

### Debug Information

The operator's output provides real-time status:
- Execute count (frames processed)
- Asyncio initialization status
- Event loop running state
- Auto-processing state
- Available methods list

## Building from Source

### Prerequisites

- Visual Studio 2019 or later (Windows)
- Xcode Command Line Tools (macOS)
- CMake 3.15 or later
- TouchDesigner SDK
- Python development headers

### Build Steps

1. Clone the repository
2. Run the build script:
   ```powershell
   # Windows
   .\build.ps1
   ```
   ```bash
   # macOS/Linux
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```
3. The plugin will be built and copied to the test directory

### Continuous Integration

The project includes GitHub Actions workflows for:
- **CI**: Automatic builds on Windows and macOS for every push and pull request
- **Release**: Automatic building and publishing of artifacts when a release is created

Release artifacts are automatically built and attached to GitHub releases for easy download.

### Development

The project structure:
- `src/AsyncioDAT.cpp/h`: Main operator implementation
- `src/py_bindings.cpp/h`: Python C API bindings
- `test/`: TouchDesigner test files and examples
- `CMakeLists.txt`: CMake build configuration

## License

This project is licensed under the MIT License. See LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Support

For questions and support:
- Check the example files in the `test/` directory
- Review the comprehensive test script: `test/asyncio_test.py`
- Open an issue on the project repository
