# AsyncioDAT Quick Start Example
# Copy this code into a textDAT in TouchDesigner and run it

import asyncio
import time
import random

print("AsyncioDAT Quick Start Example")
print("=" * 40)

# Make sure we have the AsyncioDAT operator
try:
    asyncio_dat = op('Asyncio1')
    print("✓ Found AsyncioDAT operator")
except:
    print("✗ AsyncioDAT operator 'Asyncio1' not found!")
    print("  Please create an AsyncioDAT operator and name it 'Asyncio1'")
    exit()

# Check if asyncio is running
if asyncio_dat.is_running():
    print("✓ Asyncio event loop is running")
else:
    print("! Initializing asyncio event loop...")
    asyncio_dat.initialize_asyncio()

# Simple async function examples
async def countdown(name, seconds):
    """Count down from seconds to zero"""
    print(f"[{name}] Starting countdown from {seconds}")
    for i in range(seconds, 0, -1):
        print(f"[{name}] {i}...")
        await asyncio.sleep(1)
    print(f"[{name}] Done!")

async def random_worker(worker_id):
    """Worker that does random amount of work"""
    work_time = random.uniform(1, 4)
    print(f"[Worker {worker_id}] Starting work (will take {work_time:.1f}s)")
    await asyncio.sleep(work_time)
    print(f"[Worker {worker_id}] Work completed!")
    return f"Result from worker {worker_id}"

async def periodic_message(interval=2.0, count=5):
    """Send periodic messages"""
    for i in range(count):
        print(f"[Periodic] Message {i+1}/{count}")
        await asyncio.sleep(interval)
    print("[Periodic] All messages sent!")

# Add tasks to the event loop
print("\nAdding async tasks to the event loop...")

# Add countdown tasks
asyncio_dat.add_task(countdown("Timer1", 3))
asyncio_dat.add_task(countdown("Timer2", 5))

# Add worker tasks
for i in range(3):
    asyncio_dat.add_task(random_worker(i))

# Add periodic task
asyncio_dat.add_task(periodic_message(1.5, 4))

print("\n✓ All tasks added!")
print("\nWatch the console output to see async tasks running concurrently.")
print("Notice how TouchDesigner stays responsive while tasks execute!")
print(f"\nAsyncio status: Running={asyncio_dat.is_running()}")
print(f"Auto-process events: {asyncio_dat.loop_running}")

# Optional: Demonstrate direct event loop access
def add_delayed_message():
    """Add a message using the event loop directly"""
    loop = asyncio_dat.get_event_loop()
    if loop:
        # Schedule a message to appear after 10 seconds
        loop.call_later(10, lambda: print("[Delayed] This message appeared after 10 seconds!"))
        print("✓ Delayed message scheduled for 10 seconds from now")

# Uncomment the next line to test delayed messages
# add_delayed_message()

print("\n" + "=" * 40)
print("Async tasks are now running in the background!")
print("Check the console to see them execute concurrently.")
