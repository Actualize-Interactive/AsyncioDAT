# Test script to verify ensure_future approach works
import asyncio
import time

# Get reference to the AsyncioDAT operator
try:
    asyncio_op = op('Asyncio1')
    print("AsyncioDAT operator found!")
except:
    print("ERROR: Could not find AsyncioDAT operator 'Asyncio1'")
    print("Make sure you have an AsyncioDAT operator in your network")
    asyncio_op = None

async def test_task(name, duration=1.0):
    """Simple async task for testing"""
    print(f"[{time.time():.2f}] Starting task: {name}")
    await asyncio.sleep(duration)
    print(f"[{time.time():.2f}] Completed task: {name}")
    return f"Result from {name}"

def test_ensure_future_method():
    """Test the ensure_future approach"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print("=== Testing ensure_future approach ===")
    
    # Check initial status
    print(f"Is running: {asyncio_op.is_running()}")
    print(f"Loop running: {asyncio_op.loop_running}")
    print(f"Initialized: {asyncio_op.asyncio_initialized}")
    
    # Initialize if needed
    if not asyncio_op.is_running():
        print("Initializing asyncio...")
        result = asyncio_op.initialize_asyncio()
        print(f"Initialize result: {result}")
    
    # Check status after init
    print(f"After init - Is running: {asyncio_op.is_running()}")
    print(f"After init - Loop running: {asyncio_op.loop_running}")
    print(f"After init - Initialized: {asyncio_op.asyncio_initialized}")
    
    # Try to add a task
    print("\nAdding test tasks...")
    coro1 = test_task("Test1", 2.0)
    coro2 = test_task("Test2", 1.5)
    coro3 = test_task("Test3", 1.0)
    
    success1 = asyncio_op.add_task(coro1)
    success2 = asyncio_op.add_task(coro2)
    success3 = asyncio_op.add_task(coro3)
    
    print(f"Task 1 added: {success1}")
    print(f"Task 2 added: {success2}")
    print(f"Task 3 added: {success3}")
    
    print("\nTasks added! They should start executing automatically.")
    print("Watch the console for async task output...")

def test_create_task_method():
    """Test the create_task method directly"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print("\n=== Testing create_task method ===")
    
    try:
        # Try using create_task directly
        coro = test_task("CreateTaskTest", 1.0)
        task = asyncio_op.create_task(coro)
        print(f"create_task returned: {task}")
        print(f"Task type: {type(task)}")
    except Exception as e:
        print(f"create_task failed: {e}")

def run_tests():
    """Run all tests"""
    test_ensure_future_method()
    test_create_task_method()

# Auto-run tests when script is executed
print("Running AsyncioDAT ensure_future tests...")
run_tests()
