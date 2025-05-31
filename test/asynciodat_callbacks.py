# AsyncioDAT Python Callbacks

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



def on_initialize(asyncioDat, success):
	""""Called when the AsyncioDAT is initialized."""
	print("Initialized:", asyncioDat, type(asyncioDat))
	try:
		import grpc
		print("imported grpc on initialze successfully!")
	except error as e:
		print(e)

	
	asyncioDat.set_plugin('testOnInitPlugin', TestOnInitPlugin())
	
	
def on_startup(asyncioDat, success):
	print("On Startup!!!")
	try:
		import grpc
		print("imported grpc on startup successfully!")
	except error as e:
		print(e)

	if asyncioDat.has_plugin('test_plugin'):
		plugin = asyncioDat.plugins.test_plugin
		add_method = test_service_pb2_grpc.add_CalculatorServiceServicer_to_server
		plugin.add_servicer(add_method, CalculatorServiceServicer())

	
def on_pre_shutdown(asyncioDat, success, info):
	"""Called when the AsyncioDAT is shutdown."""
	pass

def on_post_shutdown(asyncioDat, success, info):
	"""Called after the AsyncioDAT has been shutdown."""
	pass