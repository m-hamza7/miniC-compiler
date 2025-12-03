# MiniC Compiler

MiniC is an educational, lightweight compiler front-end for a simplified C-like language. This repository contains a complete implementation of the compiler front-end (lexer, parser, semantic analysis) plus a small interpreter and a Flask web UI that visualizes each compiler phase.

Live interface is served locally via Flask and displays token streams, AST, symbol/function tables, semantic issues, and program output.

---

## Key Features

- Lexical analysis using PLY (lex)
- Syntax analysis with operator precedence rules (yacc)
- Semantic analysis with symbol table, scoping and type checks
- Simple interpreter to execute validated MiniC programs
- Interactive Flask web UI with phase visualizations (tokens, AST, symbol/function tables, issues, output)
- Themed cyber-blue HUD UI (rounded & modern)

## Repository Structure

- `app.py` — Flask application and routes
- `minic_compiler_new.py` — Working compiler (lexer/parser/semantic/interpreter)
- `minic_compiler.py` — Alternate/older compiler version
- `templates/index.html` — Web UI template and client JavaScript
- `static/style.css` — Cyber-blue theme styles
- `requirements.txt` — Python dependencies
- `THEME_CHANGES.md` — Notes describing theme updates and design choices

## Quick Start (Windows PowerShell)

1. Create virtual environment and activate:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
```

2. Install dependencies:

```powershell
pip install -r requirements.txt
```

3. Run the Flask app (development):

```powershell
python app.py
```

4. Open browser to: `http://127.0.0.1:5000`


## Usage

- Paste or edit MiniC source code in the editor area.
- Click "Load Example" to populate a sample program.
- Click "Compile & Execute" to run the code through the compiler pipeline.
- Use tabs to inspect:
  - Lexical Analysis (token stream)
  - Syntax Analysis (AST)
  - Semantic Analysis (symbol/function tables and issues)
  - Output (program output and compilation summary)

## Example MiniC Program

```c
func add(a:int, b:int):int {
    return a + b;
}

func factorial(n:int):int {
    if (n <= 1) return 1;
    else return n * factorial(n - 1);
}

var x:int = 5;
var y:int = 10;
print(add(x,y));
print(factorial(5));
```

## Developer Notes

- The project focuses on teaching compiler front-end concepts; it's not production-grade.
- The web UI uses client-side JavaScript to call the `/compile` endpoint; ensure the Flask server is running and accessible.
- If any UI controls (buttons/tabs) stop working after edits, check the `templates/index.html` script for unclosed strings or missing function definitions (common cause when embedding multi-line example code).

## Contributing

Contributions are welcome. Please open issues for bugs or feature requests and submit pull requests with clear descriptions.

## License

This repository is provided for educational use. License: MIT (add LICENSE file if needed).

---

_Last updated: November 28, 2025_
