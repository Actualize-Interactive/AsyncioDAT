import grpc
import asyncio
import time
from grpc_service_plugin import GRPCServicePlugin

from calculator_servicer import CalculatorServiceServicer
from proto import test_service_pb2_grpc

def on_create(asyncio_dat):
    """
    This function is called when the AsyncioDat instance is created.
    It sets up the gRPC server and adds it as a plugin to the AsyncioDat instance.
    """
    print('on_create called with asyncio_dat:', asyncio_dat)
    
    # Create and add the plugin to the asyncio_dat instance
    test_plugin = GRPCServicePlugin()
    asyncio_dat.add_plugin('test_plugin', test_plugin)
    
    # Start the gRPC server asynchronously
    async def start_server():
        await test_plugin.start_grpc_server()
    

    add_method = test_service_pb2_grpc.add_CalculatorServiceServicer_to_server
    test_plugin.add_servicer(add_method, CalculatorServiceServicer())

    asyncio_dat.create_task(start_server())


    