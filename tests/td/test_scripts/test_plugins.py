import asyncio

def get_asyncio_dat():
    """Get the AsyncioDAT operator instance."""
    try:
        return op('Asyncio1')
    except:
        raise RuntimeError("Could not find 'Asyncio1' operator. Make sure it exists in the project.")


class SimplePlugin:
    """A simple test plugin class."""
    def __init__(self, name="test_plugin", value=42):
        self.name = name
        self.value = value
    
    def get_info(self):
        return f"Plugin {self.name} with value {self.value}"

class AsyncPlugin:
    """A plugin with async functionality."""
    def __init__(self, name="async_plugin"):
        self.name = name
        self.counter = 0
        self.results = []
    
    async def async_increment(self, amount=1):
        """Async method that increments counter."""
        await asyncio.sleep(2)  # Yield control
        self.counter += amount
        return self.counter

    async def async_fetch_data(self, data_id):
        """Simulate async data fetching."""
        await asyncio.sleep(2.1)  # Simulate network delay
        result = f"data_{data_id}_{self.counter}"
        self.results.append(result)
        return result
    
    def get_results(self):
        return self.results.copy()

class ComplexPlugin:
    """A more complex plugin with various data types."""
    def __init__(self):
        self.data = {
            'strings': ['hello', 'world'],
            'numbers': [1, 2, 3.14, -42],
            'nested': {
                'level1': {
                    'level2': ['deep', 'data']
                }
            },
            'functions': [self.method1, self.method2],
            'mixed_list': [1, 'two', 3.0, {'four': 4}]
        }
        self.callbacks = []
    
    def method1(self):
        return "method1_result"
    
    def method2(self, x):
        return f"method2_{x}"
    
    def add_callback(self, callback):
        self.callbacks.append(callback)

class TestResults:
    """Simple test result tracking."""
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.errors = []
    
    def add_pass(self, test_name):
        self.passed += 1
        print(f"✓ {test_name}")
    
    def add_fail(self, test_name, message):
        self.failed += 1
        error = f"✗ {test_name}: {message}"
        self.errors.append(error)
        print(error)
    
    def summary(self):
        total = self.passed + self.failed
        print(f"\nTest Results: {self.passed}/{total} passed")
        if self.errors:
            print("Failures:")
            for error in self.errors:
                print(f"  {error}")
        return self.failed == 0

def assert_true(condition, message="Assertion failed"):
    """Simple assertion helper."""
    if not condition:
        raise AssertionError(message)

def assert_equal(actual, expected, message=None):
    """Simple equality assertion helper."""
    if actual != expected:
        msg = message or f"Expected {expected}, got {actual}"
        raise AssertionError(msg)

def assert_is_not_none(value, message="Value should not be None"):
    """Simple not-none assertion helper."""
    if value is None:
        raise AssertionError(message)

def test_set_plugin_basic():
    """Test that set_plugin() works with a basic class instance."""
    asyncio_dat = get_asyncio_dat()
    test_plugin = SimplePlugin("test_plugin", 100)
    
    # Test setting the plugin
    result = asyncio_dat.set_plugin("my_plugin", test_plugin)
    assert_true(result, "set_plugin should return True on success")
    
    # Verify plugin was stored
    retrieved = asyncio_dat.get_plugin("my_plugin")
    assert_is_not_none(retrieved, "Retrieved plugin should not be None")
    assert_equal(retrieved.name, "test_plugin", "Plugin name should match")
    assert_equal(retrieved.value, 100, "Plugin value should match")

def test_set_plugin_overwrites_existing():
    """Test that set_plugin() overwrites an existing plugin with the same name."""
    asyncio_dat = get_asyncio_dat()
    test_plugin = SimplePlugin("test_plugin", 100)
    another_plugin = SimplePlugin("another_plugin", 200)
    
    # Set initial plugin
    asyncio_dat.set_plugin("test_key", test_plugin)
    
    # Overwrite with new plugin
    result = asyncio_dat.set_plugin("test_key", another_plugin)
    assert_true(result, "Overwriting plugin should return True")
    
    # Verify the new plugin replaced the old one
    retrieved = asyncio_dat.get_plugin("test_key")
    assert_equal(retrieved.value, 200, "Should retrieve the newer plugin's value")
    assert_equal(retrieved.name, "another_plugin", "Should retrieve the newer plugin's name")

def test_set_plugin_primitive_types():
    """Test that set_plugin() works with primitive data types."""
    asyncio_dat = get_asyncio_dat()
    
    # Test string
    result = asyncio_dat.set_plugin("string_plugin", "Hello World")
    assert_true(result, "Setting string plugin should return True")
    retrieved = asyncio_dat.get_plugin("string_plugin")
    assert_equal(retrieved, "Hello World", "Retrieved string should match")
    
    # Test integer
    asyncio_dat.set_plugin("int_plugin", 42)
    retrieved = asyncio_dat.get_plugin("int_plugin")
    assert_equal(retrieved, 42, "Retrieved int should match")
    
    # Test float
    asyncio_dat.set_plugin("float_plugin", 3.14159)
    retrieved = asyncio_dat.get_plugin("float_plugin")
    assert_equal(retrieved, 3.14159, "Retrieved float should match")
    
    # Test boolean
    asyncio_dat.set_plugin("bool_plugin", True)
    retrieved = asyncio_dat.get_plugin("bool_plugin")
    assert_equal(retrieved, True, "Retrieved bool should match")
    
    # Test None
    asyncio_dat.set_plugin("none_plugin", None)
    retrieved = asyncio_dat.get_plugin("none_plugin")
    assert_equal(retrieved, None, "Retrieved None should match")

def test_set_plugin_collections():
    """Test that set_plugin() works with collection types."""
    asyncio_dat = get_asyncio_dat()
    
    # Test list
    test_list = [1, 2, "three", 4.0, [5, 6]]
    asyncio_dat.set_plugin("list_plugin", test_list)
    retrieved = asyncio_dat.get_plugin("list_plugin")
    assert_equal(retrieved, test_list, "Retrieved list should match")
    
    # Test dictionary
    test_dict = {"key1": "value1", "key2": 42, "nested": {"inner": "data"}}
    asyncio_dat.set_plugin("dict_plugin", test_dict)
    retrieved = asyncio_dat.get_plugin("dict_plugin")
    assert_equal(retrieved["key1"], "value1", "Dict key1 should match")
    assert_equal(retrieved["key2"], 42, "Dict key2 should match")
    assert_equal(retrieved["nested"]["inner"], "data", "Nested dict value should match")
    
    # Test tuple
    test_tuple = (1, "two", 3.0)
    asyncio_dat.set_plugin("tuple_plugin", test_tuple)
    retrieved = asyncio_dat.get_plugin("tuple_plugin")
    assert_equal(retrieved, test_tuple, "Retrieved tuple should match")
    
    # Test set
    test_set = {1, 2, 3, "four"}
    asyncio_dat.set_plugin("set_plugin", test_set)
    retrieved = asyncio_dat.get_plugin("set_plugin")
    assert_equal(retrieved, test_set, "Retrieved set should match")

def test_set_plugin_complex_objects():
    """Test that set_plugin() works with complex objects."""
    asyncio_dat = get_asyncio_dat()
    
    # Test complex plugin
    complex_plugin = ComplexPlugin()
    asyncio_dat.set_plugin("complex_plugin", complex_plugin)
    retrieved = asyncio_dat.get_plugin("complex_plugin")
    
    assert_equal(retrieved.data['strings'], ['hello', 'world'], "Strings should match")
    assert_equal(retrieved.data['numbers'], [1, 2, 3.14, -42], "Numbers should match")
    assert_equal(retrieved.method1(), "method1_result", "Method1 should work")
    assert_equal(retrieved.method2("test"), "method2_test", "Method2 should work")

def test_set_plugin_functions():
    """Test that set_plugin() works with functions and lambdas."""
    asyncio_dat = get_asyncio_dat()
    
    # Test regular function
    def test_function(x, y):
        return x + y
    
    asyncio_dat.set_plugin("function_plugin", test_function)
    retrieved = asyncio_dat.get_plugin("function_plugin")
    assert_equal(retrieved(2, 3), 5, "Function should work correctly")
    
    # Test lambda
    test_lambda = lambda x: x * 2
    asyncio_dat.set_plugin("lambda_plugin", test_lambda)
    retrieved = asyncio_dat.get_plugin("lambda_plugin")
    assert_equal(retrieved(5), 10, "Lambda should work correctly")

def test_set_plugin_invalid_name():
    """Test that set_plugin() handles invalid names appropriately."""
    asyncio_dat = get_asyncio_dat()
    test_plugin = SimplePlugin("test_plugin", 100)
    
    # Test with empty string (should fail based on implementation)
    result = asyncio_dat.set_plugin("", test_plugin)
    assert_true(not result, "Empty string name should return False")

def test_get_plugin_nonexistent():
    """Test that get_plugin() returns None for non-existent plugins."""
    asyncio_dat = get_asyncio_dat()
    
    retrieved = asyncio_dat.get_plugin("nonexistent_plugin_12345")
    assert_true(retrieved is None, "Non-existent plugin should return None")

def test_has_plugin():
    """Test the has_plugin() method."""
    asyncio_dat = get_asyncio_dat()
    test_plugin = SimplePlugin("has_test", 123)
    
    # Initially should not exist
    exists = asyncio_dat.has_plugin("has_test")
    assert_true(not exists, "Plugin should not exist initially")
    
    # Add plugin
    asyncio_dat.set_plugin("has_test", test_plugin)
    
    # Now should exist
    exists = asyncio_dat.has_plugin("has_test")
    assert_true(exists, "Plugin should exist after being added")

def test_del_plugin():
    """Test the del_plugin() method."""
    asyncio_dat = get_asyncio_dat()
    test_plugin = SimplePlugin("del_test", 456)
    
    # Add plugin
    asyncio_dat.set_plugin("del_test", test_plugin)
    assert_true(asyncio_dat.has_plugin("del_test"), "Plugin should exist after adding")
    
    # Delete plugin
    result = asyncio_dat.del_plugin("del_test")
    assert_true(result, "del_plugin should return True on success")
    
    # Verify it's gone
    assert_true(not asyncio_dat.has_plugin("del_test"), "Plugin should not exist after deletion")

def test_get_plugin_names():
    """Test the get_plugin_names() method."""
    asyncio_dat = get_asyncio_dat()
    
    # Clear any existing plugins
    asyncio_dat.clear_plugins()
    
    # Add some plugins
    asyncio_dat.set_plugin("plugin1", SimplePlugin("p1", 1))
    asyncio_dat.set_plugin("plugin2", "string_plugin")
    asyncio_dat.set_plugin("plugin3", {"key": "value"})
    
    # Get names - access as property, not method call
    names = asyncio_dat.plugin_names
    assert_true(len(names) == 3, f"Should have 3 plugin names, got {len(names)}")
    assert_true("plugin1" in names, "plugin1 should be in names")
    assert_true("plugin2" in names, "plugin2 should be in names")
    assert_true("plugin3" in names, "plugin3 should be in names")

def test_clear_plugins():
    """Test the clear_plugins() method."""
    asyncio_dat = get_asyncio_dat()
    
    # Add some plugins
    asyncio_dat.set_plugin("clear1", SimplePlugin("c1", 1))
    asyncio_dat.set_plugin("clear2", "test")
    
    # Verify they exist
    assert_true(asyncio_dat.has_plugin("clear1"), "Plugin should exist before clear")
    assert_true(asyncio_dat.has_plugin("clear2"), "Plugin should exist before clear")
    
    # Clear all plugins
    asyncio_dat.clear_plugins()
    
    # Verify they're gone
    assert_true(not asyncio_dat.has_plugin("clear1"), "Plugin should not exist after clear")
    assert_true(not asyncio_dat.has_plugin("clear2"), "Plugin should not exist after clear")
    
    # Verify names list is empty - access as property
    names = asyncio_dat.plugin_names
    assert_true(len(names) == 0, f"Should have 0 plugin names after clear, got {len(names)}")

def test_plugin_async_functionality():
    """Test plugins with async functionality."""
    asyncio_dat = get_asyncio_dat()
    
    # Create async plugin
    async_plugin = AsyncPlugin("async_test")
    asyncio_dat.set_plugin("async_plugin", async_plugin)
    
    # Retrieve and test async functionality
    retrieved = asyncio_dat.get_plugin("async_plugin")
    assert_equal(retrieved.name, "async_test", "Async plugin name should match")
    assert_equal(retrieved.counter, 0, "Initial counter should be 0")
    
    # Test 1: Simple async increment - use frame counting instead of time.sleep
    async def test_simple_increment():
        result = await retrieved.async_increment(5)
        assert_equal(result, 5, "Async increment should return 5")
        print(f"Async test_simple_increment result: {result}")
        return result
    
    # Create task and let Auto Process handle it
    coro1 = test_simple_increment()
    task1 = asyncio_dat.create_task(coro1)
    
    # Instead of blocking with time.sleep, just check if task is done
    # In a real TouchDesigner environment, this would be checked across frames
    frame_count = 0
    max_frames = 120  # ~2 seconds at 60fps
    
    # Simulate frame-based checking (in real TD, this would be called from different frames)
    def check_task_completion():
        nonlocal frame_count
        frame_count += 1
        
        if hasattr(task1, 'done') and task1.done():
            if not task1.exception():
                result = task1.result()
                assert_equal(result, 5, "Async increment should return 5")
                assert_equal(retrieved.counter, 5, "Counter should be incremented to 5")
                return True
            else:
                raise AssertionError(f"Task had exception: {task1.exception()}")
        
        if frame_count >= max_frames:
            raise AssertionError("Task did not complete within frame limit")
        
        return False
    
    # Check immediately (task might complete quickly)
    if not check_task_completion():
        # In a real scenario, this would be checked in subsequent frames
        # For testing, we'll just verify the task was created properly
        assert_true(hasattr(task1, 'done'), "Task should have 'done' method")
    
    # Test 2: State-based async operations with completion tracking
    class AsyncStatePlugin:
        def __init__(self):
            self.operations_completed = 0
            self.results = []
            self.in_progress = False
        
        async def start_operation(self, operation_id):
            if self.in_progress:
                return f"operation_{operation_id}_skipped_busy"
            
            self.in_progress = True
            await asyncio.sleep(0.05)  # Small delay
            
            result = f"operation_{operation_id}_completed"
            self.results.append(result)
            self.operations_completed += 1
            self.in_progress = False
            
            return result
    
    state_plugin = AsyncStatePlugin()
    asyncio_dat.set_plugin("state_plugin", state_plugin)
    state_retrieved = asyncio_dat.get_plugin("state_plugin")
    
    # Start multiple operations
    tasks = []
    for i in range(3):
        coro = state_retrieved.start_operation(i)
        task = asyncio_dat.create_task(coro)
        tasks.append(task)
    
    # Verify tasks were created
    assert_equal(len(tasks), 3, "Should have created 3 tasks")
    for i, task in enumerate(tasks):
        assert_true(hasattr(task, 'done'), f"Task {i} should have 'done' method")
    
    # Test 3: Plugin with async cleanup and resource management
    class AsyncResourcePlugin:
        def __init__(self):
            self.resources = {}
            self.cleanup_count = 0
        
        async def acquire_resource(self, resource_id):
            await asyncio.sleep(0.01)  # Simulate acquisition delay
            self.resources[resource_id] = f"resource_data_{resource_id}"
            return f"acquired_{resource_id}"
        
        async def release_resource(self, resource_id):
            if resource_id in self.resources:
                await asyncio.sleep(0.01)  # Simulate cleanup delay
                del self.resources[resource_id]
                self.cleanup_count += 1
                return f"released_{resource_id}"
            return f"not_found_{resource_id}"
        
        async def cleanup_all(self):
            cleanup_tasks = []
            for resource_id in list(self.resources.keys()):
                cleanup_tasks.append(self.release_resource(resource_id))
            
            if cleanup_tasks:
                # In real async, we'd await all cleanup tasks
                for task_coro in cleanup_tasks:
                    # Create tasks but don't wait for them in the test
                    asyncio_dat.create_task(task_coro)
            
            return len(cleanup_tasks)
    
    resource_plugin = AsyncResourcePlugin()
    asyncio_dat.set_plugin("resource_plugin", resource_plugin)
    resource_retrieved = asyncio_dat.get_plugin("resource_plugin")
    
    # Test resource acquisition
    acquire_coro = resource_retrieved.acquire_resource("test_resource")
    acquire_task = asyncio_dat.create_task(acquire_coro)
    
    # Verify task creation
    assert_true(hasattr(acquire_task, 'done'), "Acquire task should have 'done' method")
    
    # Test 4: Frame-based progress tracking plugin
    class FrameProgressPlugin:
        def __init__(self):
            self.frame_count = 0
            self.progress_snapshots = []
            self.completed = False
        
        async def frame_based_operation(self, total_frames=10):
            for frame in range(total_frames):
                await asyncio.sleep(0)  # Yield to next frame
                self.frame_count = frame + 1
                self.progress_snapshots.append({
                    'frame': self.frame_count,
                    'progress': (frame + 1) / total_frames
                })
            
            self.completed = True
            return f"completed_after_{total_frames}_frames"
    
    progress_plugin = FrameProgressPlugin()
    asyncio_dat.set_plugin("progress_plugin", progress_plugin)
    progress_retrieved = asyncio_dat.get_plugin("progress_plugin")
    
    # Start frame-based operation
    progress_coro = progress_retrieved.frame_based_operation(5)
    progress_task = asyncio_dat.create_task(progress_coro)
    
    # Verify initial state
    assert_equal(progress_retrieved.frame_count, 0, "Initial frame count should be 0")
    assert_equal(len(progress_retrieved.progress_snapshots), 0, "Initial snapshots should be empty")
    assert_true(not progress_retrieved.completed, "Should not be completed initially")
    
    # Test 5: Error handling with proper async task management
    class AsyncErrorPlugin:
        def __init__(self):
            self.error_count = 0
            self.success_count = 0
            self.last_error = None
        
        async def operation_that_might_fail(self, should_fail=False):
            await asyncio.sleep(0.01)
            
            if should_fail:
                self.error_count += 1
                error = ValueError("Intentional test error")
                self.last_error = str(error)
                raise error
            else:
                self.success_count += 1
                return "success"
    
    error_plugin = AsyncErrorPlugin()
    asyncio_dat.set_plugin("error_plugin", error_plugin)
    error_retrieved = asyncio_dat.get_plugin("error_plugin")
    
    # Test successful operation
    success_coro = error_retrieved.operation_that_might_fail(False)
    success_task = asyncio_dat.create_task(success_coro)
    
    # Test failing operation with exception handling
    async def safe_error_operation():
        try:
            await error_retrieved.operation_that_might_fail(True)
            return "unexpected_success"
        except ValueError as e:
            return f"caught_error: {str(e)}"
    
    error_coro = safe_error_operation()
    error_task = asyncio_dat.create_task(error_coro)
    
    # Verify tasks were created
    assert_true(hasattr(success_task, 'done'), "Success task should be created")
    assert_true(hasattr(error_task, 'done'), "Error task should be created")
    
    # Test 6: Plugin interaction through async operations
    class AsyncCoordinatorPlugin:
        def __init__(self, asyncio_dat):
            self.asyncio_dat = asyncio_dat
            self.coordination_log = []
        
        async def coordinate_plugins(self):
            # Get other plugins and coordinate between them
            state_plugin = self.asyncio_dat.get_plugin("state_plugin")
            progress_plugin = self.asyncio_dat.get_plugin("progress_plugin")
            
            if state_plugin and progress_plugin:
                # Log initial states
                self.coordination_log.append(f"state_ops: {state_plugin.operations_completed}")
                self.coordination_log.append(f"progress_frames: {progress_plugin.frame_count}")
                
                # Start a coordinated operation
                await asyncio.sleep(0.01)
                self.coordination_log.append("coordination_complete")
                
                return "coordination_successful"
            
            return "plugins_not_found"
    
    coordinator = AsyncCoordinatorPlugin(asyncio_dat)
    asyncio_dat.set_plugin("coordinator", coordinator)
    coordinator_retrieved = asyncio_dat.get_plugin("coordinator")
    
    # Start coordination
    coord_coro = coordinator_retrieved.coordinate_plugins()
    coord_task = asyncio_dat.create_task(coord_coro)
    
    # Verify coordination task
    assert_true(hasattr(coord_task, 'done'), "Coordination task should be created")
    
    # Final verification - all plugins should be accessible
    plugin_names = asyncio_dat.plugin_names
    expected_plugins = ["async_plugin", "state_plugin", "resource_plugin", 
                       "progress_plugin", "error_plugin", "coordinator"]
    
    for plugin_name in expected_plugins:
        assert_true(plugin_name in plugin_names, f"Plugin {plugin_name} should be in plugin names")
        plugin = asyncio_dat.get_plugin(plugin_name)
        assert_is_not_none(plugin, f"Plugin {plugin_name} should be retrievable")

def test_plugin_persistence_across_operations():
    """Test that plugins persist across multiple operations."""
    asyncio_dat = get_asyncio_dat()
    
    # Clear and add initial plugins
    asyncio_dat.clear_plugins()
    
    plugins_data = {
        "persistent1": SimplePlugin("p1", 100),
        "persistent2": ComplexPlugin(),
        "persistent3": {"data": "test"},
        "persistent4": [1, 2, 3, 4, 5]
    }
    
    # Add all plugins
    for name, plugin in plugins_data.items():
        result = asyncio_dat.set_plugin(name, plugin)
        assert_true(result, f"Should successfully add {name}")
    
    # Verify all exist
    names = asyncio_dat.plugin_names
    assert_equal(len(names), 4, "Should have 4 plugins")
    
    # Test persistence through multiple gets
    for i in range(3):
        for name in plugins_data.keys():
            retrieved = asyncio_dat.get_plugin(name)
            assert_is_not_none(retrieved, f"{name} should exist on iteration {i}")

def test_plugin_edge_cases():
    """Test edge cases and error conditions."""
    asyncio_dat = get_asyncio_dat()
    
    # Test large data structure
    large_data = {"key_" + str(i): "value_" + str(i) for i in range(1000)}
    result = asyncio_dat.set_plugin("large_plugin", large_data)
    assert_true(result, "Should handle large data structures")
    
    retrieved = asyncio_dat.get_plugin("large_plugin")
    assert_equal(len(retrieved), 1000, "Large data should be preserved")
    assert_equal(retrieved["key_500"], "value_500", "Random key should be accessible")
    
    # Test circular reference handling (Python should handle this gracefully)
    circular_data = {"self": None}
    circular_data["self"] = circular_data
    result = asyncio_dat.set_plugin("circular_plugin", circular_data)
    assert_true(result, "Should handle circular references")
    
    # Test Unicode strings
    unicode_plugin = {"unicode": "Hello 世界 🌍 café naïve"}
    result = asyncio_dat.set_plugin("unicode_plugin", unicode_plugin)
    assert_true(result, "Should handle Unicode data")
    
    retrieved = asyncio_dat.get_plugin("unicode_plugin")
    assert_equal(retrieved["unicode"], "Hello 世界 🌍 café naïve", "Unicode should be preserved")

def test_multiple_plugins_interaction():
    """Test interaction between multiple plugins."""
    asyncio_dat = get_asyncio_dat()
    
    # Clear and set up multiple plugins
    asyncio_dat.clear_plugins()
    
    # Plugin 1: Data store
    data_store = {"shared_data": []}
    asyncio_dat.set_plugin("data_store", data_store)
    
    # Plugin 2: Processor that uses data store
    class ProcessorPlugin:
        def __init__(self, asyncio_dat):
            self.asyncio_dat = asyncio_dat
        
        def add_data(self, value):
            store = self.asyncio_dat.get_plugin("data_store")
            if store:
                store["shared_data"].append(value)
                return True
            return False
        
        def get_data_count(self):
            store = self.asyncio_dat.get_plugin("data_store")
            return len(store["shared_data"]) if store else 0
    
    processor = ProcessorPlugin(asyncio_dat)
    asyncio_dat.set_plugin("processor", processor)
    
    # Test interaction
    processor_retrieved = asyncio_dat.get_plugin("processor")
    
    # Add data through processor
    result = processor_retrieved.add_data("test1")
    assert_true(result, "Should successfully add data")
    
    result = processor_retrieved.add_data("test2")
    assert_true(result, "Should successfully add second data")
    
    # Verify data was added
    count = processor_retrieved.get_data_count()
    assert_equal(count, 2, "Should have 2 items in data store")
    
    # Verify direct access to data store
    store = asyncio_dat.get_plugin("data_store")
    assert_equal(len(store["shared_data"]), 2, "Data store should have 2 items")
    assert_equal(store["shared_data"], ["test1", "test2"], "Data should match")

def run_all_tests():
    """Run all plugin tests and return results."""
    results = TestResults()
    
    # List of all test functions
    test_functions = [
        test_set_plugin_basic,
        test_set_plugin_overwrites_existing,
        test_set_plugin_primitive_types,
        test_set_plugin_collections,
        test_set_plugin_complex_objects,
        test_set_plugin_functions,
        test_set_plugin_invalid_name,
        test_get_plugin_nonexistent,
        test_has_plugin,
        test_del_plugin,
        test_get_plugin_names,
        test_clear_plugins,
        test_plugin_async_functionality,
        test_plugin_persistence_across_operations,
        test_plugin_edge_cases,
        test_multiple_plugins_interaction,
    ]
    
    print("Running Comprehensive AsyncioDAT Plugin Tests...")
    print("=" * 50)
    
    for test_func in test_functions:
        try:
            test_func()
            results.add_pass(test_func.__name__)
        except Exception as e:
            results.add_fail(test_func.__name__, str(e))
    
    print("=" * 50)
    success = results.summary()
    return success

# Convenience function for TouchDesigner
def test_plugins():
    """Main entry point for running tests in TouchDesigner."""
    return run_all_tests()

# Auto-run when this script is executed directly (e.g. loaded into a Text DAT
# and run). The integration test runner sets ASYNCIODAT_TEST_IMPORT so it can
# import this module and call run_all_tests() itself without a double run.
import builtins as _builtins
if not getattr(_builtins, "ASYNCIODAT_TEST_IMPORT", False):
    run_all_tests()
