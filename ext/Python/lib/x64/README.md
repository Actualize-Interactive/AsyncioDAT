# Python Libraries for Windows

This directory should contain the Python 3.11.1 library files for Windows x64:

- `python311.lib` - Python 3.11 import library  
- `python311_d.lib` - Python 3.11 debug import library (optional)
- `python3.lib` - Python limited API library (optional)

These files can be obtained from a Python 3.11.1 installation or from the official Python distribution.

The CMakeLists.txt is configured to link against `python311.lib` from this directory.