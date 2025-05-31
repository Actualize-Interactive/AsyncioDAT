#!/usr/bin/env python3
"""
Simple test script for the AsyncioDAT gRPC Calculator service.

This script runs a quick validation of the gRPC service without user interaction.
"""

import asyncio
import grpc

# prepend modules/proto to sys.path to import proto files
import sys
sys.path.insert(0, 'modules/proto')

import test_service_pb2
import test_service_pb2_grpc


async def run_tests():
    """Run automated tests against the gRPC service."""
    server_address = 'localhost:50051'
    
    print(f"Testing gRPC Calculator Service at {server_address}")
    print("=" * 50)
    
    try:
        async with grpc.aio.insecure_channel(server_address) as channel:
            stub = test_service_pb2_grpc.CalculatorServiceStub(channel)
            
            # Test 1: Add operation
            print("Test 1: Add 15 + 25")
            add_response = await stub.Add(test_service_pb2.AddRequest(a=15, b=25))
            print(f"  Result: {add_response.result}")
            print(f"  Message: {add_response.message}")
            assert add_response.result == 40, f"Expected 40, got {add_response.result}"
            print("  ✓ PASSED\n")
            
            # Test 2: Multiply operation
            print("Test 2: Multiply 8 * 7")
            multiply_response = await stub.Multiply(test_service_pb2.MultiplyRequest(a=8, b=7))
            print(f"  Result: {multiply_response.result}")
            print(f"  Message: {multiply_response.message}")
            assert multiply_response.result == 56, f"Expected 56, got {multiply_response.result}"
            print("  ✓ PASSED\n")
            
            # Test 3: Echo operation
            print("Test 3: Echo 'AsyncioDAT Test'")
            echo_response = await stub.Echo(test_service_pb2.EchoRequest(text="AsyncioDAT Test"))
            print(f"  Echo: {echo_response.echo}")
            print(f"  Timestamp: {echo_response.timestamp}")
            assert "AsyncioDAT Test" in echo_response.echo, "Echo response doesn't contain input text"
            print("  ✓ PASSED\n")

            # Test 5: SetStringPar operation
            print("Test 4: SetStringPar to 'Hello from gRPC client!!!'")
            set_string_par_response = await stub.SetStringPar(test_service_pb2.SetStringParRequest(value="Hello from gRPC client!!!"))
            print(f"  Success: {set_string_par_response.success}")
            print(f"  Message: {set_string_par_response.message}")
            assert set_string_par_response.success, "SetStringPar failed"
            print("  ✓ PASSED\n")

            # Test 5: Streaming calculator
            print("Test 5: Streaming Calculator (multiple operations)")
            
            async def test_requests():
                # Test addition
                yield test_service_pb2.CalculatorRequest(
                    operation=test_service_pb2.CalculatorRequest.Operation.ADD,
                    a=10, b=5
                )
                # Test subtraction
                yield test_service_pb2.CalculatorRequest(
                    operation=test_service_pb2.CalculatorRequest.Operation.SUBTRACT,
                    a=20, b=8
                )
                # Test multiplication
                yield test_service_pb2.CalculatorRequest(
                    operation=test_service_pb2.CalculatorRequest.Operation.MULTIPLY,
                    a=6, b=7
                )
                # Test division
                yield test_service_pb2.CalculatorRequest(
                    operation=test_service_pb2.CalculatorRequest.Operation.DIVIDE,
                    a=24, b=6
                )
                # Test division by zero
                yield test_service_pb2.CalculatorRequest(
                    operation=test_service_pb2.CalculatorRequest.Operation.DIVIDE,
                    a=10, b=0
                )
            
            responses = []
            async for response in stub.Calculator(test_requests()):
                responses.append(response)
                if response.success:
                    print(f"  ✓ {response.message} (Result: {response.result})")
                else:
                    print(f"  ! {response.message}")
            
            # Validate responses
            assert len(responses) == 5, f"Expected 5 responses, got {len(responses)}"
            assert responses[0].result == 15, "Addition failed"
            assert responses[1].result == 12, "Subtraction failed"
            assert responses[2].result == 42, "Multiplication failed"
            assert responses[3].result == 4, "Division failed"
            assert not responses[4].success, "Division by zero should fail"
            print("  ✓ ALL STREAMING TESTS PASSED\n")
            
            print("🎉 All tests completed successfully!")
            print("The AsyncioDAT gRPC Calculator service is working correctly.")
            
    except grpc.RpcError as e:
        print(f"❌ gRPC Error: {e}")
        print("Make sure the AsyncioDAT server with gRPC plugin is running.")
        return False
    except AssertionError as e:
        print(f"❌ Test Assertion Failed: {e}")
        return False
    except Exception as e:
        print(f"❌ Unexpected Error: {e}")
        return False
    
    return True


if __name__ == "__main__":
    print("AsyncioDAT gRPC Service Test")
    
    try:
        success = asyncio.run(run_tests())
        if success:
            print("\n✅ All tests passed!")
        else:
            print("\n❌ Some tests failed!")
            exit(1)
    except KeyboardInterrupt:
        print("\n⚠️  Tests interrupted by user.")
    except Exception as e:
        print(f"\n💥 Fatal error: {e}")
        exit(1)
