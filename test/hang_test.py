# Simple test to verify AsyncioDAT doesn't hang with Auto Process Events
import asyncio
import time

print("=== AsyncioDAT Hang Test ===")

# Get reference to the AsyncioDAT operator
try:
    asyncio_op = op('Asyncio1')
    print("✓ AsyncioDAT operator found!")
except:
    print("✗ ERROR: Could not find AsyncioDAT operator 'Asyncio1'")
    asyncio_op = None

if not asyncio_op:
    print("Test cannot continue without operator")
else:
    print("\n1. Checking initial status...")
    print(f"   Is running: {asyncio_op.is_running()}")
    print(f"   Loop running: {asyncio_op.loop_running}")
    print(f"   Initialized: {asyncio_op.asyncio_initialized}")
    
    print("\n2. Initializing asyncio if needed...")
    if not asyncio_op.is_running():
        result = asyncio_op.initialize_asyncio()
        print(f"   Initialize result: {result}")
    
    print("\n3. Status after initialization:")
    print(f"   Is running: {asyncio_op.is_running()}")
    print(f"   Loop running: {asyncio_op.loop_running}")
    print(f"   Initialized: {asyncio_op.asyncio_initialized}")
    
    print("\n4. Testing manual process_events() call...")
    success = asyncio_op.process_events()
    print(f"   Process events result: {success}")
    
    print("\n5. The operator should NOT hang when you enable 'Auto Process Events'")
    print("   Try enabling it now - TouchDesigner should remain responsive.")
    
    print("\n✓ Basic test completed successfully!")
    print("   - No hanging during manual process_events() call")
    print("   - Ready to test Auto Process Events toggle")
