# MiniC Compiler - Interactive Web Application

A fully functional compiler front-end for the **MiniC** programming language, built with Python, PLY (Python Lex-Yacc), and Flask. This web application demonstrates all major compiler phases with interactive visualizations.

## 🎯 Project Overview

MiniC is a simplified programming language designed to mimic the basic structure and syntax of the C family. This compiler implements:

1. **Lexical Analysis** - Tokenization of source code
2. **Syntax Analysis** - Grammar validation and AST generation
3. **Semantic Analysis** - Symbol table management, type checking, and scope validation
4. **Code Execution** - Simple interpreter for validated programs

## 🚀 Features

### Language Features
- **Keywords**: `if`, `else`, `while`, `for`, `return`, `func`, `var`, `int`, `float`, `bool`, `true`, `false`, `print`
- **Data Types**: `int`, `float`, `bool`
- **Operators**: Arithmetic (`+`, `-`, `*`, `/`), Logical (`&&`, `||`, `!`), Relational (`==`, `!=`, `<`, `<=`, `>`, `>=`)
- **Control Flow**: if-else statements, while loops, for loops
- **Functions**: User-defined functions with parameters and return values
- **I/O**: `print()` statements
- **Comments**: Single-line comments using `//`

### Compiler Features
- ✅ Complete lexical analysis with token stream visualization
- ✅ Syntax analysis with Abstract Syntax Tree (AST) generation
- ✅ Semantic analysis with symbol table and type checking
- ✅ Detailed error reporting with line numbers
- ✅ Warning system for potential issues
- ✅ Code execution and output display
- ✅ Interactive web interface with beautiful UI

## 📋 Requirements

- Python 3.7 or higher
- Flask 3.0.0
- PLY (Python Lex-Yacc) 3.11

## 🔧 Installation

1. **Clone or navigate to the project directory**:
```bash
cd f:\F\BCS-7E\CC\miniC
```

2. **Install required packages**:
```bash
pip install -r requirements.txt
```

## ▶️ Running the Application

1. **Start the Flask server**:
```bash
python app.py
```

2. **Open your browser** and navigate to:
```
http://localhost:5000
```

3. **Start compiling MiniC code!**

## 📝 Example MiniC Program

```c
// Function to add two numbers
func add(a:int, b:int):int {
    return a + b;
}

// Recursive factorial function
func factorial(n:int):int {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

// Main program
var x:int = 5;
var y:int = 10;
var result:int;

result = add(x, y);
print(result);

var fact:int = factorial(5);
print(fact);
```

## 🎨 Web Interface Features

### 1. Code Editor
- Syntax-highlighted input area
- Load example code
- Clear functionality
- Compile button

### 2. Lexical Analysis Tab
- Token stream table with type, value, line, and position
- Token count statistics
- Lexical error reporting

### 3. Syntax Analysis Tab
- JSON-formatted Abstract Syntax Tree (AST)
- Visual representation of program structure
- Syntax error reporting

### 4. Semantic Analysis Tab
- **Symbol Table**: Shows all variables with type, scope, and initialization status
- **Function Table**: Lists all functions with parameters and return types
- Type checking errors and warnings
- Scope validation

### 5. Output Tab
- Program execution output
- Compilation summary with statistics
- Consolidated error and warning display

## 🏗️ Project Structure

```
miniC/
├── app.py                 # Flask web application
├── minic_compiler.py      # MiniC compiler implementation
├── requirements.txt       # Python dependencies
├── README.md             # This file
├── templates/
│   └── index.html        # Web interface template
└── static/
    └── style.css         # Stylesheet for web interface
```

## 🔍 Compiler Phases

### Phase 1: Lexical Analysis
- Tokenizes input into meaningful symbols
- Recognizes keywords, identifiers, literals, and operators
- Handles comments and whitespace
- Reports illegal characters

### Phase 2: Syntax Analysis
- Validates grammar rules
- Builds Abstract Syntax Tree (AST)
- Implements operator precedence
- Detects syntax errors with line numbers

### Phase 3: Semantic Analysis
- Symbol table management with scoping
- Type checking for expressions and assignments
- Function signature validation
- Scope checking (global vs. local)
- Initialization tracking
- Type compatibility checking

### Phase 4: Code Execution
- Simple interpreter for validated programs
- Runtime output capture
- Error handling during execution

## 🎓 Educational Value

This project demonstrates:
- **Compiler Construction**: All major front-end phases
- **Lexical Analysis**: Using PLY's lexer (lex)
- **Parsing**: Using PLY's parser (yacc) with grammar rules
- **Semantic Analysis**: Symbol tables, type systems, and scope management
- **Web Development**: Flask framework for interactive visualization
- **Software Engineering**: Clean code structure and error handling

## 🛠️ Technologies Used

- **Python**: Core programming language
- **PLY (Python Lex-Yacc)**: Lexer and parser generator
- **Flask**: Web framework for the interface
- **HTML/CSS/JavaScript**: Frontend development
- **JSON**: Data interchange format

## 📊 Supported Grammar

### Variable Declaration
```c
var x:int = 5;
var y:float;
var flag:bool = true;
```

### Function Declaration
```c
func functionName(param1:type1, param2:type2):returnType {
    // function body
    return value;
}
```

### Control Flow
```c
// If-Else
if (condition) {
    // statements
} else {
    // statements
}

// While Loop
while (condition) {
    // statements
}

// For Loop
for (init; condition; update) {
    // statements
}
```

### Expressions
```c
var result:int = (a + b) * c;
var isValid:bool = (x > 5) && (y < 10);
```

## 🐛 Error Handling

The compiler provides detailed error messages for:
- Lexical errors (illegal characters)
- Syntax errors (grammar violations)
- Semantic errors (type mismatches, undeclared variables)
- Runtime errors (during execution)

## 🎯 Future Enhancements

Potential improvements:
- Code optimization phase
- Intermediate code generation
- Assembly/bytecode generation
- More data types (strings, arrays)
- Advanced control structures (switch-case, break, continue)
- Better error recovery
- Syntax highlighting in the editor
- AST visualization with tree diagrams

## 👨‍💻 Development

### Testing
Test your compiler with various MiniC programs:
- Valid programs to ensure correct compilation
- Programs with errors to test error reporting
- Edge cases and boundary conditions

### Debugging
The web interface displays detailed information for each phase, making it easy to debug issues in your MiniC programs.

## 📄 License

This is an educational project for compiler construction learning.

## 🙏 Acknowledgments

- PLY (Python Lex-Yacc) library by David Beazley
- Flask web framework
- Compiler design principles from standard CS curricula

---

**Built for BCS-7E Compiler Construction Project**

Enjoy exploring compiler phases with MiniC! 🚀
