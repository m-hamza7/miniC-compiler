# MiniC Compiler

MiniC is an educational, lightweight compiler front-end for a simplified C-like language. This repository contains a complete implementation of the compiler front-end (lexer, parser, semantic analysis) plus a small interpreter and a Flask web UI that visualizes each compiler phase.

Live interface is served locally via Flask and displays token streams, AST, symbol/function tables, semantic issues, and program output.

---

## Key Features

- Lexical analysis (handwritten lexer for the C++ backend / PLY-based lexer in the Python reference)
- Syntax analysis with operator precedence rules (recursive-descent in C++ / yacc-style in Python reference)
- Semantic analysis with symbol table, scoping and type checks
- Simple interpreter to execute validated MiniC programs (built-in `print` and functions)
- Two backends:
  - `backend_cpp/` — single-file handwritten C++ backend executable (`minic_backend.exe`) that reads MiniC from stdin and writes JSON to stdout
  - `minic_compiler_new.py` — working Python implementation kept as a fallback and reference
- Interactive Flask web UI with phase visualizations (tokens, AST, symbol/function tables, issues, output)
- Themed rounded modern HUD UI (updated from earlier cyber-blue JARVIS styling)

## Repository Structure

- `app.py` — Flask application and routes (the `/compile` route attempts to invoke the C++ backend executable and falls back to the Python compiler when necessary)
- `minic_compiler_new.py` — Working Python compiler (lexer/parser/semantic/interpreter)
- `minic_compiler.py` — Alternate / older compiler version (legacy)
- `backend_cpp/` — C++ backend sources, build scripts and samples
  - `main.cpp` — single-file C++ implementation (lexer, parser, AST, semantic, interpreter, JSON emitter)
  - `CMakeLists.txt`, `build.ps1` — optional MSVC/CMake/PowerShell build support
  - `sample.minic` — example input files used during development
  - `minic_backend.exe` / `main.exe` — example built executables (may be present in workspace; rebuild locally for reproducible result)
  - `result*.json` — sample JSON outputs produced by the backend when run against samples
- `templates/index.html` — Web UI template and client JavaScript
- `static/style.css` — UI theme and styles (rounded modern HUD)
- `requirements.txt` — Python dependencies for running the Flask app
- `THEME_CHANGES.md` — notes describing the UI theme changes

---

## Quick Start (MSYS2 MinGW64 — recommended for reproducible g++ builds)

Open the "MSYS2 MinGW 64-bit" shell (recommended) or use the "MSYS2 MinGW 64-bit" VS Code Terminal profile.

1. Update MSYS2 and install the mingw-w64 toolchain:

```bash
# Run in MSYS2 MINGW64 shell
pacman -Syu
# If pacman asks to close the shell, close and reopen the same MINGW64 shell, then run:
pacman -S --needed mingw-w64-x86_64-toolchain
```

2. Build the C++ backend using g++ (from the project folder):

```bash
cd /f/F/BCS-7E/CC/miniC/backend_cpp
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o minic_backend.exe
```

3. Run the backend with a sample to generate JSON output:

```bash
./minic_backend.exe < sample.minic > result.json
```

4. Start the Flask app (use PowerShell / cmd or the project virtual environment):

```powershell
# from the project root
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
python app.py
```

5. Open your browser to http://127.0.0.1:5000 and use the web UI. The `/compile` endpoint will attempt to call `backend_cpp/minic_backend.exe`; if the executable is missing or the subprocess fails the server falls back to the Python compiler (`minic_compiler_new.py`).

---

## Alternative Build Options (Windows PowerShell / MSVC)

- A PowerShell helper `backend_cpp/build.ps1` and a `CMakeLists.txt` are provided for users who prefer to use MSVC/CMake. Ensure you have CMake and the Visual Studio Desktop build tools installed, then run the PowerShell script from an elevated PowerShell session or a Developer Command Prompt.

- If you prefer to build with MinGW from PowerShell but not the MSYS shell, call the g++ binary directly (adjust path to your MSYS2 installation):

```powershell
& 'C:\msys64\mingw64\bin\g++.exe' -std=c++17 -O2 -Wall -Wextra f:\F\BCS-7E\CC\miniC\backend_cpp\main.cpp -o f:\F\BCS-7E\CC\miniC\backend_cpp\minic_backend.exe
```

- A VS Code build task for g++ may already exist (Tasks -> Run Build Task) and is configured to use `C:\msys64\mingw64\bin\g++.exe` in this workspace. Use it if you want a one-click build inside VS Code.

---

## Running & Testing

- Use the web UI to paste or load `backend_cpp/sample.minic` and press "Compile & Execute". The UI will display tokens, AST JSON (pretty-printed), symbol table, function table, semantic issues (errors/warnings) and program output.

- If you prefer to call the C++ backend manually, pipe code to stdin and read JSON on stdout as shown above.

- To compare behavior with the Python compiler, run `minic_compiler_new.py` on the same samples and compare outputs.

---

## Developer Notes

- The C++ backend is implemented as a single-file handwritten compiler (`main.cpp`) for portability and easy review. It includes a small lexer, recursive-descent parser with operator precedence, AST representation, semantic pass, a minimal interpreter and a JSON emitter.

- `app.py` was updated to prefer invoking the C++ backend executable located at `backend_cpp/minic_backend.exe`. If that executable is not present or fails, `app.py` falls back to the Python compiler (`minic_compiler_new.py`). This makes the Flask UI usable even without a local C++ build.

- Frontend theme and templates were updated to a rounded modern HUD look (see `static/style.css` and `THEME_CHANGES.md`). No changes to the Flask routes were required — the frontend JavaScript will consume the JSON produced by either backend.

- The C++ source includes a small fix to strip a leading UTF-8 BOM from input to avoid reporting illegal-character tokens for files saved with BOM.

---

## Troubleshooting

- If `g++` is not found in the MSYS2 MinGW64 shell, ensure you installed the `mingw-w64-x86_64-toolchain` package and that you're running the MINGW64 subsystem (check `echo $MSYSTEM` — it should print `MINGW64`).

- If the VS Code integrated terminal opens to `MSYS` instead of `MINGW64`, use the "MSYS2 MinGW 64‑bit" terminal profile or run `export MSYSTEM=MINGW64 && exec bash -l -i` in the MSYS prompt to restart it as MINGW64.

- If `app.py` still falls back to Python even after building the exe, check file permissions and that `backend_cpp/minic_backend.exe` is reachable from the Flask process. Also inspect Flask server logs for the subprocess output.

---

## Contributing

Contributions are welcome. Please open issues for bugs or feature requests and submit pull requests with clear descriptions. When adding tests or changes to the C++ backend, include sample inputs and expected JSON outputs for regression testing.

## License

This repository is provided for educational use. License: MIT (add LICENSE file if needed).

---

_Last updated: December 3, 2025_
