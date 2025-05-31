#!/usr/bin/env python3
"""
gRPC Calculator Client for testing AsyncioDAT gRPC integration.

This script demonstrates both unary and bidirectional streaming RPC calls
to the CalculatorService running in AsyncioDAT.
"""

import asyncio
import grpc
import sys
from modules.proto import test_service_pb2
from modules.proto import test_service_pb2_grpc


async def test_unary_calls(stub):
    """Test unary RPC calls (Add, Multiply, Echo)."""
    print("=== Testing Unary RPC Calls ===")
    
    # Test Add
    print("\n1. Testing Add operation...")
    add_request = test_service_pb2.AddRequest(a=10, b=5)
    add_response = await stub.Add(add_request)
    print(f"Add Result: {add_response.result}, Message: {add_response.message}")
    
    # Test Multiply
    print("\n2. Testing Multiply operation...")
    multiply_request = test_service_pb2.MultiplyRequest(a=7, b=6)
    multiply_response = await stub.Multiply(multiply_request)
    print(f"Multiply Result: {multiply_response.result}, Message: {multiply_response.message}")
    
    # Test Echo
    print("\n3. Testing Echo operation...")
    echo_request = test_service_pb2.EchoRequest(text="Hello AsyncioDAT!")
    echo_response = await stub.Echo(echo_request)
    print(f"Echo Result: {echo_response.echo}, Timestamp: {echo_response.timestamp}")


async def interactive_calculator(stub):
    """Interactive bidirectional streaming calculator."""
    print("\n=== Interactive Calculator (Bidirectional Streaming) ===")
    print("Operations: add, subtract, multiply, divide, quit")
    print("Format: <operation> <number1> <number2>")
    print("Example: add 5 3")
    print("-" * 50)
    
    async def request_generator():
        """Generate calculator requests from user input."""
        while True:
            try:
                # Get user input
                user_input = input("\nCalculator> ").strip().lower()
                
                if user_input in ['quit', 'exit', 'q']:
                    print("Exiting calculator...")
                    break
                
                parts = user_input.split()
                if len(parts) != 3:
                    print("Invalid format. Use: <operation> <number1> <number2>")
                    continue
                
                operation_str, a_str, b_str = parts
                
                try:
                    a = float(a_str)
                    b = float(b_str)
                except ValueError:
                    print("Invalid numbers. Please enter valid numbers.")
                    continue
                
                # Map operation string to proto enum
                operation_map = {
                    'add': test_service_pb2.CalculatorRequest.Operation.ADD,
                    'subtract': test_service_pb2.CalculatorRequest.Operation.SUBTRACT,
                    'multiply': test_service_pb2.CalculatorRequest.Operation.MULTIPLY,
                    'divide': test_service_pb2.CalculatorRequest.Operation.DIVIDE,
                }
                
                if operation_str not in operation_map:
                    print("Unknown operation. Use: add, subtract, multiply, divide")
                    continue
                
                # Create and yield the request
                request = test_service_pb2.CalculatorRequest(
                    operation=operation_map[operation_str],
                    a=a,
                    b=b
                )
                yield request
                
            except KeyboardInterrupt:
                print("\nExiting calculator...")
                break
            except EOFError:
                print("\nExiting calculator...")
                break
    
    try:
        # Start the bidirectional streaming call
        response_stream = stub.Calculator(request_generator())
        
        # Process responses
        async for response in response_stream:
            if response.success:
                print(f"✓ Result: {response.result}")
                print(f"  Message: {response.message}")
            else:
                print(f"✗ Error: {response.message}")
                
    except grpc.RpcError as e:
        print(f"RPC Error: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")


async def main():
    """Main function to run the gRPC client."""
    server_address = 'localhost:50051'
    
    print(f"Connecting to gRPC server at {server_address}...")
    
    try:
        # Create async gRPC channel
        async with grpc.aio.insecure_channel(server_address) as channel:
            # Create stub
            stub = test_service_pb2_grpc.CalculatorServiceStub(channel)
            
            print("Connected successfully!")
            
            # Test unary calls first
            await test_unary_calls(stub)
            
            # Then run interactive calculator
            await interactive_calculator(stub)
            
    except grpc.RpcError as e:
        print(f"Failed to connect to gRPC server: {e}")
        print("Make sure the AsyncioDAT gRPC server is running.")
    except KeyboardInterrupt:
        print("\nClient interrupted by user.")
    except Exception as e:
        print(f"Unexpected error: {e}")


if __name__ == "__main__":
    print("AsyncioDAT gRPC Calculator Client")
    print("=" * 40)
    
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nGoodbye!")
    except Exception as e:
        print(f"Fatal error: {e}")
        sys.exit(1)
