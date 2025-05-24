# Test script to verify the new processEvents implementation works
import asyncio
import time

print("=== Working Pattern Test ===")

# Get reference to the AsyncioDAT operator
try:
    asyncio_op = op('Asyncio1')
    print("✓ AsyncioDAT operator found!")
except:
    print("✗ ERROR: Could not find AsyncioDAT operator 'Asyncio1'")
    asyncio_op = None

# Simple test task that prints when it runs
async def test_task(name, delay=1.0):
    """Simple async task that should actually execute"""
    print(f"🚀 TASK {name} STARTED at {time.time():.2f}")
    await asyncio.sleep(delay)
    print(f"✅ TASK {name} COMPLETED at {time.time():.2f}")
    return f"Result from {name}"

async def immediate_task(name):
    """Task that completes immediately"""
    print(f"⚡ IMMEDIATE TASK {name} EXECUTED at {time.time():.2f}")
    return f"Immediate result from {name}"

if asyncio_op:
    print(f"\n📋 Initial Status:")
    print(f"   Is running: {asyncio_op.is_running()}")
    print(f"   Loop running: {asyncio_op.loop_running}")
    print(f"   Initialized: {asyncio_op.asyncio_initialized}")
    
    # Initialize if needed
    if not asyncio_op.is_running():
        print("\n🔧 Initializing asyncio...")
        result = asyncio_op.initialize_asyncio()
        print(f"   Initialize result: {result}")
    
    print(f"\n📋 Status after init:")
    print(f"   Is running: {asyncio_op.is_running()}")
    print(f"   Loop running: {asyncio_op.loop_running}")
    print(f"   Initialized: {asyncio_op.asyncio_initialized}")
    
    # Add some test tasks
    print(f"\n➕ Adding test tasks...")
    
    # Add immediate tasks that should execute quickly
    success1 = asyncio_op.add_task(immediate_task("Immediate1"))
    success2 = asyncio_op.add_task(immediate_task("Immediate2"))
    
    # Add delayed tasks
    success3 = asyncio_op.add_task(test_task("Delayed1", 0.5))
    success4 = asyncio_op.add_task(test_task("Delayed2", 1.0))
    
    print(f"   Immediate1 added: {success1}")
    print(f"   Immediate2 added: {success2}")
    print(f"   Delayed1 added: {success3}")
    print(f"   Delayed2 added: {success4}")
    
    print(f"\n⚙️  Manual processing test...")
    for i in range(5):
        print(f"   Manual process {i+1}...")
        result = asyncio_op.process_events()
        print(f"   Process result: {result}")
        time.sleep(0.1)  # Small delay between manual processing
    
    print(f"\n✅ Test complete!")
    print(f"🔄 Now enable 'Auto Process Events' to see continuous execution")
    print(f"   Tasks should execute automatically every frame!")
else:
    print("❌ Cannot run test without AsyncioDAT operator")

print("\n" + "="*50)
print("If you see task execution messages above, the new")
print("processEvents implementation is working correctly!")
print("="*50)
