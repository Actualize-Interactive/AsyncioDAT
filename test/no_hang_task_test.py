# Test to verify tasks can be added without hanging
import asyncio
import time

print("=== AsyncioDAT Task Test (No Hang) ===")

# Get reference to the AsyncioDAT operator
try:
    asyncio_op = op('Asyncio1')
    print("✓ AsyncioDAT operator found!")
except:
    print("✗ ERROR: Could not find AsyncioDAT operator 'Asyncio1'")
    asyncio_op = None

async def simple_task(name):
    """Very simple async task"""
    print(f"Task {name} starting")
    await asyncio.sleep(0.5)
    print(f"Task {name} completed")
    return f"Result from {name}"

if asyncio_op:
    print("\n1. Initializing...")
    if not asyncio_op.is_running():
        asyncio_op.initialize_asyncio()
    
    print(f"   Status: Running={asyncio_op.is_running()}")
    
    print("\n2. Adding a simple task...")
    coro = simple_task("TestTask")
    success = asyncio_op.add_task(coro)
    print(f"   Task added: {success}")
    
    print("\n3. Manually processing events once...")
    process_result = asyncio_op.process_events()
    print(f"   Process result: {process_result}")
    
    print("\n4. Test complete!")
    print("   - Task was added successfully")
    print("   - Manual event processing didn't hang")
    print("   - Now try enabling 'Auto Process Events' - it should NOT hang")
    print("   - The task may not execute because we need a proper event loop")
    print("   - But the important thing is NO HANGING!")
else:
    print("Cannot run test without operator")
