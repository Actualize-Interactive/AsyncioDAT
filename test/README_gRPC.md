# AsyncioDAT gRPC Calculator Service

This project demonstrates a complete gRPC service implementation within AsyncioDAT, featuring a Calculator service with both unary and bidirectional streaming operations.

## Project Structure

```
test/
├── proto/
│   └── test_service.proto          # Protocol buffer definition
├── modules/
│   ├── proto/
│   │   ├── test_service_pb2.py     # Generated message classes
│   │   └── test_service_pb2_grpc.py # Generated service classes
│   ├── calculator_servicer.py      # CalculatorService implementation
│   ├── grpc_service_plugin.py      # gRPC server plugin
│   └── __init__.py
├── asyncio_dat_callbacks.py        # AsyncioDAT callbacks; on_start starts the gRPC server
├── grpc_client.py                  # Interactive gRPC client
├── test_grpc_service.py            # Automated test script
└── config.toml                     # AsyncioDAT configuration
```

## Features

### gRPC Service Methods

1. **Add** (Unary): Adds two numbers
2. **Multiply** (Unary): Multiplies two numbers  
3. **Echo** (Unary): Echoes text with timestamp
4. **Calculator** (Bidirectional Streaming): Interactive calculator with operations:
   - ADD: Addition
   - SUBTRACT: Subtraction  
   - MULTIPLY: Multiplication
   - DIVIDE: Division (with zero-division protection)

## Setup and Usage

### 1. Install Dependencies
```powershell
uv sync
```

### 2. Regenerate Proto Files (if needed)
```powershell
uv run python -m grpc_tools.protoc --python_out=. --grpc_python_out=. --proto_path=proto proto\test_service.proto
# Move generated files to modules/proto/ and fix imports
```

### 3. Run AsyncioDAT with gRPC Plugin
The gRPC server automatically starts when AsyncioDAT loads the `asyncio_dat_callbacks.py` callback module (its `on_start` function starts the server). The server runs on port **50051**.

### 4. Test the Service

#### Automated Testing
```powershell
uv run python test_grpc_service.py
```

#### Interactive Client
```powershell
uv run python grpc_client.py
```

The interactive client provides:
- Automated unary operation tests
- Interactive bidirectional streaming calculator
- Commands: `add`, `sub`, `mul`, `div`, `quit`

## Example Usage

### Unary Operations
```python
# Add operation
add_request = test_service_pb2.AddRequest(a=10, b=5)
add_response = await stub.Add(add_request)
# Result: 15, Message: "Added 10 + 5 = 15"

# Echo operation  
echo_request = test_service_pb2.EchoRequest(text="Hello!")
echo_response = await stub.Echo(echo_request)
# Echo: "Echo: Hello!", Timestamp: 1735123456
```

### Streaming Calculator
```
Calculator> add 10 5
✓ Performed addition: 10.0 and 5.0 = 15.0 (Result: 15.0)

Calculator> div 10 0  
✗ Error: Division by zero

Calculator> quit
```

## Configuration

The gRPC paths are added to `config.toml`:

```toml
[main]
paths = [
    "modules",
    "modules/proto"
]

[asyncio]
callback_module_path = "asyncio_dat_callbacks.py"
```

## AsyncioDAT Integration

The `asyncio_dat_callbacks.py` file demonstrates:
- Plugin architecture for AsyncioDAT
- Async gRPC server setup
- Service registration
- Lifecycle management (start/stop)

The plugin automatically:
1. Creates a gRPC server instance
2. Registers the CalculatorServiceServicer
3. Starts listening on port 50051
4. Integrates with AsyncioDAT's plugin system

## Development Notes

- All proto message names follow the pattern `<Operation>Request` and `<Operation>Response`
- The service uses async/await for all operations
- Error handling includes division by zero protection
- The bidirectional stream handles client disconnections gracefully
- Import paths are configured to work within the AsyncioDAT environment
