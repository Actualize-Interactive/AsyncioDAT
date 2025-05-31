import grpc
# import asyncio
# import time
# from calculator_servicer import CalculatorServiceServicer
# from proto import test_service_pb2
# from proto import test_service_pb2_grpc

class GRPCServicePlugin:
    """
    This class is a plugin for AsyncioDat that sets up a gRPC server
    when the AsyncioDat instance is created.
    """
    
    def __init__(self):
        self.name = 'GRPCServicePlugin'
        self.server = grpc.aio.server()
        self.port = 50051

   
    def add_servicer(self, add_method, servicer):
        add_method(servicer, self.server)

    async def start_grpc_server(self):
        """Start the async gRPC server."""
        # self.server = grpc.aio.server()
        # self.calculator_servicer = CalculatorServiceServicer()
        # test_service_pb2_grpc.add_CalculatorServiceServicer_to_server(self.calculator_servicer, self.server)
        
        listen_addr = f'[::]:{self.port}'
        self.server.add_insecure_port(listen_addr)
        print(f'Starting gRPC server on port {self.port}...')
        await self.server.start()
        print(f'gRPC server started and listening on {listen_addr}')
        
        return self.server
    
    async def stop_grpc_server(self):
        """Stop the gRPC server."""
        if self.server:
            print('Stopping gRPC server...')
            await self.server.stop(5)
            print('gRPC server stopped.')
    
    def test_method(self):
        """
        A test method that can be called to verify the plugin functionality.
        """
        print('GRPCServicePlugin method called.')
