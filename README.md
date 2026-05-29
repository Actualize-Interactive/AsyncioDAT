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

2. The DLL will be automatically copied to the `tests/td/Plugins/` directory

3. Open the test TouchDesigner file: `tests/td/test.toe`

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
  - `callback_module_path`: Path to the Python file containing the `on_start` callback function (and other lifecycle callbacks)

**Default behavior (no config.toml):**

If no `config.toml` is found beside the `.toe` file, AsyncioDAT:
- Does not prepend any extra paths to `sys.path`
- Loads lifecycle callbacks from `asyncio_dat_callbacks.py` in the working directory (optional — it's fine if the file doesn't exist)

## Usage

### Basic Setup

1. Add an AsyncioDAT operator to your TouchDesigner network
2. Ensure the "Auto Poll" parameter is enabled (default)
3. The operator will automatically initialize an asyncio event loop

### Python API

The AsyncioDAT operator exposes the following methods:

#### Core Methods

```python
# Get reference to the operator
asyncio_op = op('Asyncio1')

# Initialize asyncio (called automatically)
asyncio_op.initialize_asyncio()

# Shutdown asyncio
asyncio_op.shutdown_asyncio()

# Process events manually (if auto-polling is disabled)
asyncio_op.poll_event_loop()

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
op('Asyncio1').add_task(simple_task())
```

#### Periodic Task

```python
async def periodic_task():
    count = 0
    while count < 10:
        print(f"Periodic task tick: {count}")
        count += 1
        await asyncio.sleep(1.0)

op('Asyncio1').add_task(periodic_task())
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
op('Asyncio1').add_task(fetch_data(url))
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
    op('Asyncio1').add_task(worker(i, random.uniform(1, 3)))
```

### Operator Parameters

All parameters live on the **Asyncio** page:

- **Active**: Toggle that initializes (on) or shuts down (off) the managed event loop (default: enabled)
- **Reset Event Loop**: Pulse to shut down and reinitialize the asyncio event loop
- **Auto Poll**: Toggle automatic event polling every frame (default: enabled)
- **Add Stop Task**: Toggle that schedules `loop.stop()` each poll so the loop drains ready tasks via `run_forever` before yielding (default: off)
- **Max Status Rows**: Maximum number of status messages shown in the output table (default: 10)
- **Clear Status**: Pulse to clear the status message table
- **on_poll_begin callback active**: Toggle that enables the `on_poll_begin` callback each frame (default: off)
- **on_poll_end callback active**: Toggle that enables the `on_poll_end` callback each frame (default: off)

### Properties

- `loop_running`: Read-only boolean indicating if the event loop is active
- `asyncio_initialized`: Read-only boolean indicating if asyncio is properly initialized
- `plugin_names`: Read-only list of registered plugin names
- `plugins`: Accessor for registered plugins (e.g. `op('Asyncio1').plugins.my_plugin`)

### Lifecycle Callbacks

AsyncioDAT exposes a Python callbacks DAT with these hooks: `on_initialized`,
`on_poll_begin`, `on_poll_end`, `on_shutdown_begin`, `on_shutdown_complete`. In
addition, an `on_start(asyncio_dat)` function in the configured callback module
(`asyncio_dat_callbacks.py` by default) is called once when the first AsyncioDAT
instance is created — useful for registering plugins or starting background
services. See `tests/td/asyncio_dat_callbacks.py` for a worked example.

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

- CMake 3.25 or later
- Visual Studio 2019/2022 (Windows) or Xcode Command Line Tools (macOS)
- **Python 3.11**, provided via [uv](https://docs.astral.sh/uv/): `uv python install 3.11`

TouchDesigner embeds CPython 3.11, so the operator must be built against 3.11.
Python is **not** vendored in this repository — a uv-managed CPython ships the
headers and import library CMake needs. The TouchDesigner Custom Operator SDK
headers are included under `ext/td/include/` (see [NOTICE](NOTICE)).

### Build Steps

1. Clone the repository and install Python 3.11: `uv python install 3.11`
2. Build the operator (CMake auto-detects the uv Python 3.11):
   ```powershell
   .\build.ps1     # Windows
   ```
   ```bash
   ./build.sh      # macOS/Linux
   ```
   Or with CMake directly (see `CMakePresets.json`):
   ```bash
   cmake --preset dev
   cmake --build --preset dev --target asyncio_dat
   ```
3. The plugin is built and copied to `tests/td/Plugins/` automatically.

### Continuous Integration

The project includes GitHub Actions workflows for:
- **CI**: Automatic builds on Windows and macOS for every push and pull request to `main`
- **Release**: Automatic building and publishing of artifacts when a release is published

Release artifacts (`AsyncioDAT-windows.zip` containing `AsyncioDAT.dll`, and
`AsyncioDAT-macos.zip` containing the `AsyncioDAT.plugin` bundle) are built and
attached to GitHub releases for easy download.

### Development

The project structure:
- `src/asyncio_dat.cpp/h`: Main operator implementation
- `src/py_bindings.cpp/h`: Python C API bindings
- `ext/td/include/`: TouchDesigner Custom Operator SDK headers (see [NOTICE](NOTICE))
- `tests/cpp/`: Catch2 unit tests (no TouchDesigner required)
- `tests/python/`: pytest suite against a compiled test extension (no TouchDesigner required)
- `tests/td/`: TouchDesigner project, scripts, and the local integration harness
- `CMakeLists.txt`: CMake build configuration

## License

This project is licensed under the MIT License. See LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Support

For questions and support:
- Check the example files under `tests/td/` (e.g. `examples/quickstart.py`)
- Review the comprehensive test scripts in `tests/td/test_scripts/` (`asyncio_test.py`, `test_plugins.py`)
- See [TESTING.md](TESTING.md) for the unit tests and the local TouchDesigner integration harness
- Open an issue on the project repository
