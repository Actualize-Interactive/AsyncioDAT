# AsyncioDAT Python Callbacks
from grpc_service_plugin import GRPCServicePlugin
from calculator_servicer import CalculatorServiceServicer
from proto import test_service_pb2_grpc

class TestOnInitPlugin:
    """
    This class is a plugin for AsyncioDat that sets up a gRPC channel
    when the AsyncioDat instance is created.
    """

    def __init__(self):
        self.name = 'TestPlugin'

    def test_on_init_method(self):
        """
        A test method that can be called to verify the plugin functionality.
        """
        print('TestOnInitPlugin method called.')



def start_grpc_server(asyncio_dat):
    """ Creates a plugin and starts a gRPC server asynchronously. """

    grpc_plugin = GRPCServicePlugin()
    asyncio_dat.set_plugin('grpc_plugin', grpc_plugin)

    add_method = test_service_pb2_grpc.add_CalculatorServiceServicer_to_server
    grpc_plugin.add_servicer(add_method, CalculatorServiceServicer())

     # Start the gRPC server asynchronously
    async def start_server():
        await grpc_plugin.start_grpc_server()

    asyncio_dat.create_task(start_server())

def on_start(asyncio_dat):
    """
    This function is called when the AsyncioDat instance is created on startup of TD.

    It is only called once, when the first AsyncioDAT instance is created - if the callback
    is loaded from disk either in the default path (cwd/asyncio_dat_callbacks.py) or
    in a path specified in a config.toml file beside .toe file. 
    
    This callback does not need to be located in this dat/file. Also it is not called 
    from the loaded callbacks DAT in TouchDesigner itself therefore you cannot use 
    this function to set values or do work in TouchDesigner itself.

    This callback is only useful for loading libraries or services that will be used
    by the objects in the TouchDesigner project or for services that are loaded
    and will act on objects in the network once it is running.
    """
    print('on_create called by asyncio_dat:', asyncio_dat)

    # Create and set a plugin to the asyncio_dat instance
    # plugins can be accessed on the AsyncioDAT with the plugins member
    asyncio_dat.set_plugin('testOnInitPlugin', TestOnInitPlugin())
    # after setting the plugin, you can call get it with
    # if asyncio_dat.has_plugin('testOnInitPlugin'):
    #     asyncio_dat.get_plugin.testOnInitPlugin.test_on_init_method()

    start_grpc_server(asyncio_dat)


def on_initialized(asyncio_dat):
    """
    Called after the AsyncioDAT event loop management is initialized.

    This is not called when TouchDesigner loads an AsyncioDAT for the
    first time on startup...
  
    """
    print('on_initialized called by asyncio_dat:', asyncio_dat)

    # Since this isn't called on the first AsyncioDAT instance
    # we can use it to set up plugins or services that are needed the 
    # AsyncioDAT was deactivated and reactivated.
    # set_plugin will overwrite the existing plugin if it exists
    # so we can safely call it again. 
    asyncio_dat.set_plugin('testOnInitPlugin', TestOnInitPlugin())
    start_grpc_server(asyncio_dat)
    pass
        
def on_poll_begin(asyncio_dat):
    """Called at the beginning of polling the event loop."""
    pass

def on_poll_end(asyncio_dat):
    """Called at the end of polling the event loop."""
    pass

def on_shutdown_begin(asyncio_dat):
    """
    Called when the asyncio is shutting down.

    This is called before all plugins are cleared and before 
    the event loop is stopped.

    It is useful for cleaning up resources or stopping services.
    """
    pass

def on_shutdown_complete(asyncio_dat):
    """
    Called after the asyncio has been shutdown.
    This is called after all plugins are cleared and after
    the event loop is stopped.

    It is useful for final cleanup or logging.
    """
    pass
