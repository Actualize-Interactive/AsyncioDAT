"""
Calculator gRPC service implementation for AsyncioDAT testing.
"""
import grpc
import asyncio
import time
from proto import test_service_pb2
from proto import test_service_pb2_grpc

from td import *

class CalculatorServiceServicer(test_service_pb2_grpc.CalculatorServiceServicer):
    """
    Implementation of the CalculatorService gRPC service.
    """
    
    async def Add(self, request, context):
        """Handle Add requests."""
        result = request.a + request.b
        return test_service_pb2.AddResponse(
            result=result,
            message=f"Added {request.a} + {request.b} = {result}"
        )
    
    async def Multiply(self, request, context):
        """Handle Multiply requests."""
        result = request.a * request.b
        return test_service_pb2.MultiplyResponse(
            result=result,
            message=f"Multiplied {request.a} * {request.b} = {result}"
        )
    
    async def Echo(self, request, context):
        """Handle Echo requests."""
        timestamp = int(time.time())
        return test_service_pb2.EchoResponse(
            echo=f"Echo: {request.text}",
            timestamp=timestamp
        )

    async def SetStringPar(self, request, context):
        """Handle SetStringParam requests."""

        success = False
        try:
            comp = op('test_set_pars')
            comp.par.Str = request.value
            success = comp.par.Str.eval() == request.value
            if success:
                print("gRPC client set Str par parameter to:", request.value)
        except Exception as e:
            print(f"Error setting Str par: {e}")
            return test_service_pb2.SetStringParResponse(
                success=False,
                message=f"Error setting Str par: {e}"
            )
        
        return test_service_pb2.SetStringParResponse(
            success=success,
            message=f"Set string parameter to: {request.value}"
        )

    async def Calculator(self, request_iterator, context):
        """Handle bidirectional streaming calculator requests."""
        async for request in request_iterator:
            try:
                if request.operation == test_service_pb2.CalculatorRequest.Operation.ADD:
                    result = request.a + request.b
                    operation_str = "addition"
                    print(f"Performed addition: {request.a} + {request.b} = {result}")
                elif request.operation == test_service_pb2.CalculatorRequest.Operation.SUBTRACT:
                    result = request.a - request.b
                    operation_str = "subtraction"
                    print(f"Performed subtraction: {request.a} - {request.b} = {result}")
                elif request.operation == test_service_pb2.CalculatorRequest.Operation.MULTIPLY:
                    result = request.a * request.b
                    operation_str = "multiplication"

                elif request.operation == test_service_pb2.CalculatorRequest.Operation.DIVIDE:
                    if request.b == 0:
                        yield test_service_pb2.CalculatorResponse(
                            result=0,
                            message="Error: Division by zero",
                            success=False
                        )
                        print(f"Cannot perform division by zero: {request.a} / {request.b} = Inf")
                        continue
                    result = request.a / request.b
                    operation_str = "division"
                    print(f"Performed division: {request.a} / {request.b} = {result}")
                else:
                    yield test_service_pb2.CalculatorResponse(
                        result=0,
                        message="Error: Unknown operation",
                        success=False
                    )
                    continue
                
                yield test_service_pb2.CalculatorResponse(
                    result=result,
                    message=f"Performed {operation_str}: {request.a} and {request.b} = {result}",
                    success=True
                )
            except Exception as e:
                yield test_service_pb2.CalculatorResponse(
                    result=0,
                    message=f"Error: {str(e)}",
                    success=False
                )