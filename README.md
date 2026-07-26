# AsyncioDAT - Asyncio Event Loop for TouchDesigner

[![CI](https://github.com/Actualize-Interactive/AsyncioDAT/actions/workflows/ci.yml/badge.svg)](https://github.com/Actualize-Interactive/AsyncioDAT/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![TouchDesigner 2025.32820](https://img.shields.io/badge/TouchDesigner-2025.32820-orange.svg)

AsyncioDAT is a C++ TouchDesigner operator that provides a managed asyncio event loop for Python scripting within TouchDesigner. It enables truly asynchronous programming without blocking the main TouchDesigner thread.

## Features

- **Automatic Event Loop Management**: Initializes, manages, and tears down an asyncio event loop automatically
- **Frame-Synchronized Processing**: Processes asyncio events once per TouchDesigner frame for smooth integration
- **Non-Blocking Operations**: Enables async I/O, network requests, and other long-running operations without freezing TouchDesigner
- **Easy Python Integration**: Simple Python API for adding coroutines and managing async tasks
- **Comprehensive Error Handling**: Robust error handling and recovery mechanisms

## Requirements

- **TouchDesigner 2025.32820** or newer. The operator is built against Custom
  Operator SDK v4 and TouchDesigner's embedded CPython **3.11**.
- **Windows 10/11**, or **macOS on Apple Silicon**. The macOS build is arm64
  only, matching TouchDesigner.

## Installation

### From a release (no build required)

1. Download the asset for your platform from the
   [latest release](https://github.com/Actualize-Interactive/AsyncioDAT/releases/latest)
   and extract it:

   | Platform | Asset | Contents |
   | --- | --- | --- |
   | Windows | `AsyncioDAT-windows.zip` | `AsyncioDAT.dll` |
   | macOS | `AsyncioDAT-macos.zip` | `AsyncioDAT.plugin` bundle |

2. Put the operator in a **`Plugins/` folder beside your `.toe`**. TouchDesigner
   loads Custom Operators from a `Plugins/` directory next to the project file;
   there is no way to point it at an arbitrary directory. (A `Plugins/` folder
   in your TouchDesigner user directory — `Documents/Derivative/Plugins/` —
   works too, and makes the operator available to every project.)

   ```text
   MyProject/
   ├── MyProject.toe
   └── Plugins/
       └── AsyncioDAT.dll        # or AsyncioDAT.plugin on macOS
   ```

3. **macOS only:** the downloaded bundle carries a quarantine attribute and is
   not signed or notarized, so TouchDesigner will refuse to load it until you
   clear the attribute:

   ```bash
   xattr -dr com.apple.quarantine Plugins/AsyncioDAT.plugin
   ```

4. Open your project. On first load, TouchDesigner shows a modal asking you to
   **approve/trust** the Custom Operator — accept it. Then add an **Asyncio**
   DAT from the operator palette.

### From source

See [Building from Source](#building-from-source) below. `build.ps1` /
`build.sh` copy the freshly built operator into `tests/td/Plugins/`, so the
bundled test project (`tests/td/test.toe`) picks it up.

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

# Number of callbacks currently ready to run on the loop
pending = asyncio_op.get_callback_count()
```

#### Plugin Registry

The operator holds a dictionary of named Python objects that outlives any single
script — useful for keeping a client, a connection pool, or a service object
alive across cooks. Register them from `on_start` (see
[Lifecycle Callbacks](#lifecycle-callbacks)) and reach them from anywhere.

```python
asyncio_op = op('Asyncio1')

# Register / replace. Returns True on success; re-registering a name replaces
# the previous object and releases the reference to it.
asyncio_op.set_plugin('my_service', MyService())

# Look up. Returns None if the name is not registered.
service = asyncio_op.get_plugin('my_service')

asyncio_op.has_plugin('my_service')     # -> True
asyncio_op.del_plugin('my_service')     # -> True, or False if it wasn't there
asyncio_op.clear_plugins()              # remove all of them
```

The `plugins` property is an attribute-style view over the same registry, and
`plugin_names` lists the registered names:

```python
asyncio_op.plugins.my_service = MyService()   # same as set_plugin
service = asyncio_op.plugins.my_service       # AttributeError if not registered
del asyncio_op.plugins.my_service             # same as del_plugin

'my_service' in asyncio_op.plugins            # -> bool
asyncio_op.plugin_names                       # -> list of names
```

These methods act on whichever AsyncioDAT instance owns the event loop, so
calling them before one is active raises `RuntimeError`.

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
   - Verify "Auto Poll" is enabled
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

The operator's output table carries the most recent status messages (how many is
set by **Max Status Rows**), and the same messages go to the textport. Attach an
Info CHOP to the operator for live counters:

| Channel | Meaning |
| --- | --- |
| `event_loop_active` | 1 while asyncio is initialized |
| `event_loop_auto_poll` | 1 while **Auto Poll** is on |
| `event_loop_poll_count` | Frames polled since initialization |
| `event_loop_poll_duration` | Time spent in the last poll, in milliseconds |

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
- **CI**: builds and runs the unit suites on Windows and macOS for every push and
  pull request to `main`
- **Release**: triggered by pushing a `v*.*.*` tag. It builds, tests, and
  packages on both platforms before publishing anything, and takes the release
  notes from the matching [CHANGELOG.md](CHANGELOG.md) entry. See
  [CONTRIBUTING.md](CONTRIBUTING.md#releasing).

Release artifacts (`AsyncioDAT-windows.zip` containing `AsyncioDAT.dll`, and
`AsyncioDAT-macos.zip` containing the `AsyncioDAT.plugin` bundle) are attached
to GitHub releases for easy download.

### Development

The project structure:
- `src/asyncio_dat.cpp/h`: Main operator implementation
- `src/py_bindings.cpp/h`: Python C API bindings
- `src/config.cpp/h`: `config.toml` parsing
- `ext/td/include/`: TouchDesigner Custom Operator SDK headers (see [NOTICE](NOTICE))
- `tests/cpp/`: Catch2 unit tests (no TouchDesigner required)
- `tests/python/`: pytest suite against a compiled test extension (no TouchDesigner required)
- `tests/td/`: TouchDesigner project, scripts, and the local integration harness
- `CMakeLists.txt`: CMake build configuration

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE). Bundled
and third-party components are attributed in [NOTICE](NOTICE).

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for the build,
test, and pull request expectations. Security issues go through
[private reporting](SECURITY.md), not a public issue.

Release-by-release changes are recorded in [CHANGELOG.md](CHANGELOG.md).

## Support

For questions and support:
- Check the example files under `tests/td/` (e.g. `examples/quickstart.py`)
- Review the TouchDesigner integration suite in `tests/td/` (`asyncio_test.py`, run by `td_test_runner.py`)
- See [TESTING.md](TESTING.md) for the unit tests and the local TouchDesigner integration harness
- Open an issue on the project repository
