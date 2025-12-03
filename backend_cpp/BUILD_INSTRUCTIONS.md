Build instructions for C++ backend (MSVC - PowerShell)

Requirements:
- Visual Studio with C++ workload (MSVC)
- CMake

From PowerShell in project root (f:\F\BCS-7E\CC\miniC):

# Create build directory
mkdir backend_cpp\\build
cd backend_cpp\\build

# Configure and generate MSVC solution (x64)
cmake -G "Visual Studio 16 2019" -A x64 ..

# Build release target
cmake --build . --config Release

# After build, executable will be at:
# backend_cpp\\build\\Release\\minic_backend.exe

Notes:
- If you use Visual Studio 2022 change generator to "Visual Studio 17 2022".
- The Flask app will call backend_cpp\\minic_backend.exe automatically if present.
