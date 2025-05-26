import grpc  # This import is now accessible in the callback

class TestPlugin:
    """
    This class is a plugin for AsyncioDat that sets up a gRPC channel
    when the AsyncioDat instance is created.
    """
    
    def __init__(self):
        self.name = 'TestPlugin'
    
    def test_method(self):
        """
        A test method that can be called to verify the plugin functionality.
        """
        print('TestPlugin method called.')


       

def on_create(asyncio_dat):
    """
    This function is called when the AsyncioDat instance is created.
    It sets up the gRPC channel and stub for the AsyncioDat instance.
    """
    print('on_create called with asyncio_dat:', asyncio_dat)
    
    if grpc is not None:
        print('gRPC is available, import work!')
    else:
        print('gRPC is not available, cannot set up channel and stub.')
        return
    
    # add a plugin to the asyncio_dat instance
    asyncio_dat.add_plugin('test_plugin', TestPlugin())

    