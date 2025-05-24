# Simple AsyncioDAT Example
# This script shows basic usage of the AsyncioDAT operator

import asyncio
import time

# Example 1: Basic async function
async def hello_async(name, delay=1.0):
    """Simple async function that greets after a delay"""
    print(f"Hello {name}! (starting)")
    await asyncio.sleep(delay)
    print(f"Hello {name}! (completed after {delay}s)")
    return f"Greeting for {name}"

# Example 2: Counter with periodic updates
async def async_counter(max_count=5, interval=1.0):
    """Count from 0 to max_count with async delays"""
    for i in range(max_count):
        print(f"Count: {i}")
        await asyncio.sleep(interval)
    print("Counter finished!")

# Example 3: Simulated network request
async def simulate_network_request(url, duration=2.0):
    """Simulate a network request with async delay"""
    print(f"Fetching {url}...")
    start_time = time.time()
    await asyncio.sleep(duration)  # Simulate network delay
    end_time = time.time()
    result = f"Data from {url} (took {end_time - start_time:.1f}s)"
    print(f"Received: {result}")
    return result

# Get reference to AsyncioDAT operator
# Make sure you have an AsyncioDAT operator named 'asynciodat1' in your network
try:
    asyncio_dat = op('Asyncio1')
    print("AsyncioDAT operator found!")
except:
    print("Error: Could not find AsyncioDAT operator 'asynciodat1'")
    print("Make sure you have an AsyncioDAT operator in your network")
    asyncio_dat = None

def run_basic_example():
    """Run basic async examples"""
    if not asyncio_dat:
        print("No AsyncioDAT operator available")
        return
    
    print("Running basic async examples...")
    
    # Add some async tasks to the event loop
    asyncio_dat.add_task(hello_async("World", 2.0))
    asyncio_dat.add_task(hello_async("TouchDesigner", 1.5))
    asyncio_dat.add_task(async_counter(3, 0.5))
    asyncio_dat.add_task(simulate_network_request("https://example.com", 3.0))
    
    print("Tasks added! Watch the console for async execution...")
    print("Tasks will run concurrently without blocking TouchDesigner!")

def check_status():
    """Check the status of the AsyncioDAT operator"""
    if not asyncio_dat:
        print("No AsyncioDAT operator available")
        return
    
    print("AsyncioDAT Status:")
    print(f"- Is running: {asyncio_dat.is_running()}")
    print(f"- Loop running: {asyncio_dat.loop_running}")
    print(f"- Initialized: {asyncio_dat.asyncio_initialized}")

# Run the example automatically when this script is loaded
if asyncio_dat:
    print("=" * 50)
    print("AsyncioDAT Simple Example")
    print("=" * 50)
    check_status()
    print("\nTo run examples, call: run_basic_example()")
    print("To check status, call: check_status()")
    print("\nExample tasks will run asynchronously and won't block TouchDesigner!")
else:
    print("Please create an AsyncioDAT operator named 'asynciodat1' first")
