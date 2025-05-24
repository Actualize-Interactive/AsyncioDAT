# Test script to monitor async task execution
import asyncio
import time

print("=== AsyncioDAT Task Execution Monitor ===")

# Get reference to the AsyncioDAT operator
try:
    asyncio_op = op('Asyncio1')
    print("✓ AsyncioDAT operator found!")
except:
    print("✗ ERROR: Could not find AsyncioDAT operator 'Asyncio1'")
    asyncio_op = None

# Global counter to track task execution
task_counter = {'count': 0}

async def monitored_task(name, duration=1.0):
    """Async task that prints execution status"""
    task_counter['count'] += 1
    task_id = task_counter['count']
    
    print(f"🚀 TASK STARTED: {name} (ID: {task_id}) at {time.time():.2f}")
    
    await asyncio.sleep(duration)
    
    print(f"✅ TASK COMPLETED: {name} (ID: {task_id}) after {duration}s at {time.time():.2f}")
    return f"Result from {name} (ID: {task_id})"

async def counting_task(name, max_count=3):
    """Task that counts with visual progress"""
    task_counter['count'] += 1
    task_id = task_counter['count']
    
    print(f"🔢 COUNTER STARTED: {name} (ID: {task_id})")
    
    for i in range(max_count):
        print(f"   📊 {name} count: {i+1}/{max_count}")
        await asyncio.sleep(0.5)
    
    print(f"✅ COUNTER COMPLETED: {name} (ID: {task_id})")
    return f"Counter {name} finished"

def check_status():
    """Check the current status of the event loop"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print(f"\n📋 STATUS CHECK:")
    print(f"   Asyncio running: {asyncio_op.is_running()}")
    print(f"   Loop running: {asyncio_op.loop_running}")
    print(f"   Initialized: {asyncio_op.asyncio_initialized}")
    print(f"   Ready callbacks: {asyncio_op.get_callback_count()}")

def add_test_tasks():
    """Add several test tasks to monitor execution"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print("\n🎯 ADDING TEST TASKS...")
    
    # Initialize if needed
    if not asyncio_op.is_running():
        print("Initializing asyncio...")
        asyncio_op.initialize_asyncio()
    
    # Add tasks with different durations
    tasks_added = 0
    
    # Short task
    coro1 = monitored_task("ShortTask", 0.5)
    if asyncio_op.add_task(coro1):
        tasks_added += 1
        print(f"   ✓ Added ShortTask")
    
    # Medium task  
    coro2 = monitored_task("MediumTask", 1.0)
    if asyncio_op.add_task(coro2):
        tasks_added += 1
        print(f"   ✓ Added MediumTask")
    
    # Long task
    coro3 = monitored_task("LongTask", 2.0)
    if asyncio_op.add_task(coro3):
        tasks_added += 1
        print(f"   ✓ Added LongTask")
    
    # Counting task
    coro4 = counting_task("Counter1", 4)
    if asyncio_op.add_task(coro4):
        tasks_added += 1
        print(f"   ✓ Added Counter1")
    
    print(f"\n📊 SUMMARY: {tasks_added} tasks added to event loop")
    
    # Check status after adding tasks
    check_status()
    
    print(f"\n🔄 PROCESSING: Now enable 'Auto Process Events' to see execution!")
    print(f"   Or call manual_process() repeatedly to step through execution")

def manual_process():
    """Manually process events once"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print(f"\n⚙️  MANUAL PROCESSING...")
    
    # Check callbacks before processing
    before_count = asyncio_op.get_callback_count()
    print(f"   Ready callbacks before: {before_count}")
    
    # Process events
    success = asyncio_op.process_events()
    
    # Check callbacks after processing
    after_count = asyncio_op.get_callback_count()
    print(f"   Ready callbacks after: {after_count}")
    print(f"   Process result: {success}")
    
    if before_count > after_count:
        print(f"   ✅ Processed {before_count - after_count} callbacks!")
    elif before_count == 0:
        print(f"   ℹ️  No callbacks were ready to process")
    else:
        print(f"   ⚠️  Callback count unchanged")

def monitor_loop():
    """Monitor the event loop continuously"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print(f"\n👁️  MONITORING EVENT LOOP...")
    for i in range(10):
        callback_count = asyncio_op.get_callback_count()
        print(f"   Iteration {i+1}: {callback_count} ready callbacks")
        
        if callback_count > 0:
            manual_process()
        
        # Small delay
        import time
        time.sleep(0.1)

# Run initial setup
if asyncio_op:
    check_status()
    add_test_tasks()
    
    print(f"\n🎮 AVAILABLE COMMANDS:")
    print(f"   manual_process()  - Process events once manually")
    print(f"   check_status()    - Check current status")
    print(f"   monitor_loop()    - Monitor callback processing")
    print(f"   add_test_tasks()  - Add more test tasks")
else:
    print("Setup failed - no operator available")
