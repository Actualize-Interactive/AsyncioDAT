# Test script for the updated AsyncioDAT with manual event processing
import asyncio
import time

# Get reference to the AsyncioDAT operator
try:
    asyncio_op = op('Asyncio1')
    print("AsyncioDAT operator found!")
except:
    print("ERROR: Could not find AsyncioDAT operator 'Asyncio1'")
    asyncio_op = None

async def simple_test_task(name, duration=1.0):
    """Simple async task for testing"""
    print(f"[{time.time():.2f}] Task {name} starting")
    await asyncio.sleep(duration)
    print(f"[{time.time():.2f}] Task {name} completed after {duration}s")
    return f"Result from {name}"

async def counting_task(max_count=5):
    """Task that counts with delays"""
    for i in range(max_count):
        print(f"[{time.time():.2f}] Count: {i}")
        await asyncio.sleep(0.5)
    print(f"[{time.time():.2f}] Counting finished!")

def test_manual_processing():
    """Test the new manual event processing approach"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print("=== Testing Manual Event Processing ===")
    
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
    
    # Try to add tasks using ensure_future approach
    print("\nAdding tasks using ensure_future...")
    
    coro1 = simple_test_task("Task1", 2.0)
    coro2 = simple_test_task("Task2", 1.5)
    coro3 = counting_task(3)
    
    success1 = asyncio_op.add_task(coro1)
    success2 = asyncio_op.add_task(coro2)
    success3 = asyncio_op.add_task(coro3)
    
    print(f"Task 1 added: {success1}")
    print(f"Task 2 added: {success2}")
    print(f"Task 3 added: {success3}")
    
    print("\nTasks added! Enable 'Auto Process Events' to see them execute.")
    print("Or call manual_process() to process events manually.")

def manual_process():
    """Manually process events"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print("Manually processing events...")
    success = asyncio_op.process_events()
    print(f"Process events result: {success}")

def test_create_task_directly():
    """Test create_task method directly"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print("=== Testing create_task directly ===")
    
    try:
        coro = simple_test_task("DirectTask", 1.0)
        task = asyncio_op.create_task(coro)
        print(f"create_task returned: {task}")
        print(f"Task type: {type(task)}")
        
        if task:
            print("Task created successfully!")
    except Exception as e:
        print(f"create_task failed: {e}")

# Run tests
print("Running updated AsyncioDAT tests...")
test_manual_processing()
print()
test_create_task_directly()
print()

print("Test complete. Try enabling 'Auto Process Events' on the operator.")
print("Available manual functions:")
print("- manual_process() : Process events once")
print("- test_manual_processing() : Re-run the setup test")
print("- test_create_task_directly() : Test create_task method")
