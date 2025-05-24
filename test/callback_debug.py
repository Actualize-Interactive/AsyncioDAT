# Simple test to understand asyncio callback execution
import asyncio
import time

print("=== AsyncioDAT Callback Debug Test ===")

# Get reference to the AsyncioDAT operator
try:
    asyncio_op = op('Asyncio1')
    print("✓ AsyncioDAT operator found!")
except:
    print("✗ ERROR: Could not find AsyncioDAT operator 'Asyncio1'")
    asyncio_op = None

# Simple test function that just prints
async def debug_task(name):
    """Ultra simple async task for debugging"""
    print(f"🎯 DEBUG_TASK {name}: Starting execution")
    await asyncio.sleep(0.1)  # Very short sleep
    print(f"🎯 DEBUG_TASK {name}: After sleep")
    return f"Debug result from {name}"

def run_debug_test():
    """Run a simple debug test"""
    if not asyncio_op:
        print("No operator available")
        return
    
    print("\n1. Initializing asyncio...")
    if not asyncio_op.is_running():
        asyncio_op.initialize_asyncio()
    
    print(f"   Status: running={asyncio_op.is_running()}")
    print(f"   Ready callbacks: {asyncio_op.get_callback_count()}")
    
    print("\n2. Adding ONE simple task...")
    coro = debug_task("TEST1")
    success = asyncio_op.add_task(coro)
    print(f"   Task added: {success}")
    print(f"   Ready callbacks after add: {asyncio_op.get_callback_count()}")
    
    print("\n3. Manual processing...")
    for i in range(5):
        before = asyncio_op.get_callback_count()
        print(f"   Iteration {i+1}: {before} callbacks before processing")
        
        if before > 0:
            success = asyncio_op.process_events()
            after = asyncio_op.get_callback_count()
            print(f"   Iteration {i+1}: {after} callbacks after processing (success: {success})")
        else:
            print(f"   Iteration {i+1}: No callbacks to process")
        
        time.sleep(0.2)  # Small delay
    
    print("\n4. Final status:")
    print(f"   Ready callbacks: {asyncio_op.get_callback_count()}")

def test_native_asyncio():
    """Test what native asyncio does for comparison"""
    print("\n=== NATIVE ASYNCIO COMPARISON ===")
    
    # Create a native asyncio task for comparison
    import asyncio
    
    async def native_test():
        print("Native asyncio task starting")
        await asyncio.sleep(0.1)
        print("Native asyncio task completed")
        return "Native result"
    
    # This won't work in TouchDesigner because there's no running loop
    # But it shows what we're trying to achieve
    try:
        # This will likely fail in TouchDesigner
        task = asyncio.create_task(native_test())
        print(f"Native task created: {task}")
    except Exception as e:
        print(f"Native asyncio failed (expected): {e}")

# Run the tests
if asyncio_op:
    run_debug_test()
    test_native_asyncio()
    
    print("\n🎮 MANUAL COMMANDS:")
    print("   run_debug_test() - Run the debug test again")
    print("   test_native_asyncio() - Test native asyncio for comparison")
else:
    print("Setup failed - no operator available")
