# AsyncioDAT Test Script
# This script demonstrates how to use the AsyncioDAT operator for async programming in TouchDesigner

import asyncio
import time
import random

# Get reference to the AsyncioDAT operator
# Replace 'Asyncio1' with the actual name of your AsyncioDAT operator
asyncio_op = op('Asyncio1')

# Test 1: Basic async task
async def basic_task(name, duration=1.0):
    """A simple async task that waits for a duration"""
    print(f"Starting task: {name}")
    await asyncio.sleep(duration)
    print(f"Completed task: {name}")
    return f"Result from {name}"

# Test 2: Periodic task with counter
async def periodic_counter(interval=1.0, max_count=5):
    """A periodic task that counts up to max_count"""
    count = 0
    while count < max_count:
        print(f"Periodic counter: {count}")
        count += 1
        await asyncio.sleep(interval)
    print("Periodic counter finished")

# Test 3: Concurrent tasks
async def concurrent_worker(worker_id, work_duration):
    """Worker that simulates some async work"""
    start_time = time.time()
    print(f"Worker {worker_id} starting work...")
    
    # Simulate some async work with random delays
    for i in range(3):
        await asyncio.sleep(work_duration / 3 + random.uniform(-0.1, 0.1))
        print(f"Worker {worker_id} progress: {i+1}/3")
    
    end_time = time.time()
    print(f"Worker {worker_id} finished in {end_time - start_time:.2f}s")
    return f"Worker {worker_id} result"

# Test 4: Error handling in async tasks
async def error_prone_task():
    """Task that demonstrates error handling"""
    print("Starting error-prone task...")
    await asyncio.sleep(0.5)
    
    # Randomly succeed or fail
    if random.random() < 0.5:
        print("Task succeeded!")
        return "Success"
    else:
        print("Task will fail...")
        raise Exception("Random failure occurred")
    
# Test 5: Set parameters on a COMP operator
async def set_pars_task(comp, par_dict: dict | None = None, offset: float = 1.0):
    """Set parameters on a COMP operator with staggered delays"""
    if par_dict is None:
        return
    delay = 0.0
    for par, value in par_dict.items():
        await asyncio.sleep(offset)
        par.val = value
        delay = offset + delay
        print(f"Setting parameter {par.name} to {value} with delay {delay}s")

    print("All parameters set")
    return "Parameters updated"

# Utility functions for testing

def test_basic_functionality():
    """Test basic AsyncioDAT functionality"""
    print("=== Testing Basic Functionality ===")
    
    # Check if asyncio is initialized
    if not asyncio_op.is_running():
        print("Initializing asyncio...")
        asyncio_op.initialize_asyncio()
    
    print(f"Asyncio running: {asyncio_op.is_running()}")
    print(f"Loop running: {asyncio_op.loop_running}")
    print(f"Asyncio initialized: {asyncio_op.asyncio_initialized}")

def add_simple_task():
    """Add a simple task to the event loop"""
    print("=== Adding Simple Task ===")
    coro = basic_task("simple_test", 2.0)
    success = asyncio_op.add_task(coro)
    print(f"Task added successfully: {success}")

def add_multiple_tasks():
    """Add multiple concurrent tasks"""
    print("=== Adding Multiple Tasks ===")
    
    # Add periodic counter
    periodic_coro = periodic_counter(0.5, 8)
    asyncio_op.add_task(periodic_coro)
    
    # Add multiple workers
    for i in range(3):
        worker_coro = concurrent_worker(i, random.uniform(1.0, 3.0))
        asyncio_op.add_task(worker_coro)
    
    print("Multiple tasks added to the event loop")

def add_error_task():
    """Add a task that might fail - with proper exception handling"""
    print("=== Adding Error-Prone Task ===")
    
    async def safe_error_task():
        """Wrapper that handles exceptions properly"""
        try:
            result = await error_prone_task()
            print(f"Error-prone task result: {result}")
            return result
        except Exception as e:
            print(f"Caught exception in error-prone task: {e}")
            return None
    
    error_coro = safe_error_task()
    asyncio_op.add_task(error_coro)

def manual_process_events():
    """Manually process events (useful when auto-processing is disabled)"""
    print("=== Manually Processing Events ===")
    success = asyncio_op.process_events()
    print(f"Events processed successfully: {success}")

def get_event_loop_info():
    """Get information about the current event loop"""
    print("=== Event Loop Info ===")
    loop = asyncio_op.get_event_loop()
    if loop:
        print(f"Event loop object: {loop}")
        print(f"Loop type: {type(loop)}")
    else:
        print("No event loop available")

def test_task_creation():
    """Test creating tasks directly"""
    print("=== Testing Task Creation ===")
    coro = basic_task("direct_task", 1.5)
    task = asyncio_op.create_task(coro)
    if task:
        print(f"Task created: {task}")
        print(f"Task type: {type(task)}")
    else:
        print("Failed to create task")

def run_coroutine_directly():
    """Test running a coroutine until completion"""
    print("=== Running Coroutine Directly ===")
    coro = basic_task("direct_run", 1.0)
    try:
        result = asyncio_op.run_coroutine(coro)
        print(f"Coroutine result: {result}")
    except Exception as e:
        print(f"Error running coroutine: {e}")

# Example HTTP request task (requires aiohttp)
async def fetch_example():
    """Example HTTP request using aiohttp"""
    try:
        import aiohttp
        async with aiohttp.ClientSession() as session:
            async with session.get('https://httpbin.org/delay/1') as response:
                data = await response.json()
                print(f"HTTP request result: {data}")
                return data
    except ImportError:
        print("aiohttp not available - skipping HTTP test")
        return None
    except Exception as e:
        print(f"HTTP request failed: {e}")
        return None

def add_http_task():
    """Add an HTTP request task"""
    print("=== Adding HTTP Task ===")
    http_coro = fetch_example()
    asyncio_op.add_task(http_coro)

# Add a new function to test exception handling
def test_exception_handling():
    """Test proper exception handling in tasks"""
    print("=== Testing Exception Handling ===")
    
    async def controlled_failure():
        """Task that always fails but handles it properly"""
        try:
            await asyncio.sleep(0.1)
            raise ValueError("Controlled test exception")
        except ValueError as e:
            print(f"Handled controlled exception: {e}")
            return "Exception handled successfully"
    
    async def success_task():
        """Task that succeeds"""
        await asyncio.sleep(0.1)
        print("Success task completed")
        return "Success"
    
    # Add both tasks
    asyncio_op.add_task(controlled_failure())
    asyncio_op.add_task(success_task())
    print("Exception handling test tasks added")

def test_set_pars():
    """Test setting parameters on the AsyncioDAT operator"""
    print("=== Testing Set Parameters ===")
    
    comp = op('test_set_pars')
    if comp is None:
        print("No operator found for set_pars test")
        return

    # reset parameters to default
    comp.par.Float = comp.par.Float.default
    comp.par.Int = comp.par.Int.default
    comp.par.Str = comp.par.Str.default
    comp.par.Toggle = comp.par.Toggle.default

    par_values = {
        comp.par.Float : 1.0,
        comp.par.Int : 42,
        comp.par.Str : "Hello, World!",
        comp.par.Toggle : True
    }   

    asyncio_op.add_task(set_pars_task(comp, par_values, offset=0.5))

# Main test function that can be called from TouchDesigner
def run_all_tests():
    """Run comprehensive tests of AsyncioDAT functionality"""
    print("Starting AsyncioDAT comprehensive tests...")
    print("=" * 50)
    
    test_basic_functionality()
    print()
    
    add_simple_task()
    print()
    
    add_multiple_tasks()
    print()
    
    add_error_task()
    print()
    
    test_exception_handling()
    print()
    
    test_task_creation()
    print()
    
    # Uncomment to test HTTP functionality (requires aiohttp)
    # add_http_task()
    # print()
    
    test_set_pars()
    print()

    get_event_loop_info()
    print()
    
    print("All tests added to the event loop!")
    print("Watch the console output to see async tasks executing...")
    print("You can also manually process events by calling manual_process_events()")

# Quick test functions for interactive use
def quick_test():
    """Quick test - add a few simple tasks"""
    print("Running quick test...")
    add_simple_task()
    add_multiple_tasks()

def stress_test():
    """Stress test - add many concurrent tasks"""
    print("Running stress test...")
    for i in range(10):
        worker_coro = concurrent_worker(f"stress_{i}", random.uniform(0.5, 2.0))
        asyncio_op.add_task(worker_coro)
    print("Stress test tasks added")

# Instructions for use
# print("""
# AsyncioDAT Test Script Loaded!

# Available test functions:
# - run_all_tests()     : Run comprehensive tests
# - quick_test()        : Run a quick test with a few tasks
# - stress_test()       : Run stress test with many concurrent tasks
# - add_simple_task()   : Add a single simple task
# - add_multiple_tasks(): Add multiple concurrent tasks
# - manual_process_events(): Manually process events
# - get_event_loop_info(): Get event loop information

# Example usage in TouchDesigner Python:
# 1. Make sure you have an AsyncioDAT operator named 'asynciodat1'
# 2. Enable 'Auto Process Events' on the operator
# 3. Run: run_all_tests()
# 4. Watch the console for async task output

# The AsyncioDAT operator will automatically process events every frame,
# so you should see the async tasks executing concurrently!
# """)

run_all_tests()