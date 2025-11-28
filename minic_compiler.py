import ply.lex as lex
import ply.yacc as yacc
from collections import defaultdict
import json

# Define tokens globally
reserved = {
    'if': 'IF', 'else': 'ELSE', 'while': 'WHILE', 'for': 'FOR',
    'return': 'RETURN', 'func': 'FUNC', 'var': 'VAR',
    'int': 'INT', 'float': 'FLOAT', 'bool': 'BOOL',
    'true': 'TRUE', 'false': 'FALSE', 'print': 'PRINT',
}

tokens = [
    'IDENTIFIER', 'NUMBER', 'FLOATNUM',
    'PLUS', 'MINUS', 'TIMES', 'DIVIDE',
    'LPAREN', 'RPAREN', 'LBRACE', 'RBRACE',
    'SEMICOLON', 'COLON', 'COMMA', 'ASSIGN',
    'EQ', 'NE', 'LT', 'LE', 'GT', 'GE',
    'AND', 'OR', 'NOT'
] + list(reserved.values())

# Token rules
t_PLUS = r'\+'
t_MINUS = r'-'
t_TIMES = r'\*'
t_DIVIDE = r'/'
t_LPAREN = r'\('
t_RPAREN = r'\)'
t_LBRACE = r'\{'
t_RBRACE = r'\}'
t_SEMICOLON = r';'
t_COLON = r':'
t_COMMA = r','
t_EQ = r'=='
t_NE = r'!='
t_LE = r'<='
t_GE = r'>='
t_LT = r'<'
t_GT = r'>'
t_AND = r'&&'
t_OR = r'\|\|'
t_NOT = r'!'
t_ASSIGN = r'='
t_ignore = ' \t'

def t_FLOATNUM(t):
    r'\d+\.\d+'
    t.value = float(t.value)
    return t

def t_NUMBER(t):
    r'\d+'
    t.value = int(t.value)
    return t

def t_IDENTIFIER(t):
    r'[a-zA-Z_][a-zA-Z_0-9]*'
    t.type = reserved.get(t.value, 'IDENTIFIER')
    return t

def t_COMMENT(t):
    r'//.*'
    pass

def t_newline(t):
    r'\n+'
    t.lexer.lineno += len(t.value)

def t_error(t):
    print(f"Illegal character '{t.value[0]}'")
    t.lexer.skip(1)

# Operator precedence
precedence = (
    ('left', 'OR'),
    ('left', 'AND'),
    ('nonassoc', 'EQ', 'NE'),
    ('nonassoc', 'LT', 'LE', 'GT', 'GE'),
    ('left', 'PLUS', 'MINUS'),
    ('left', 'TIMES', 'DIVIDE'),
    ('right', 'NOT'),
    ('right', 'UMINUS'),
)

class MiniCCompiler: 
    def __init__(self):
        self.tokens_list = []
        self.errors = []
        self.warnings = []
        self.scoped_symbols = defaultdict(dict)
        self.function_table = {}
        self.current_scope = 'global'
        
    def compile(self, code):
        """Main compilation pipeline"""
        result = {
            'success': True,
            'lexical': {},
            'syntax': {},
            'semantic': {},
            'output': '',
            'errors': [],
            'warnings': []
        }
        
        # Phase 1: Lexical Analysis
        lexer_result = self.lexical_analysis(code)
        result['lexical'] = lexer_result
        
        if lexer_result.get('errors'):
            result['success'] = False
            result['errors'].extend(lexer_result['errors'])
            return result
        
        # Phase 2: Syntax Analysis
        parser_result = self.syntax_analysis(code)
        result['syntax'] = parser_result
        
        if parser_result.get('errors'):
            result['success'] = False
            result['errors'].extend(parser_result['errors'])
            return result
        
        # Phase 3: Semantic Analysis
        semantic_result = self.semantic_analysis(parser_result.get('ast'))
        result['semantic'] = semantic_result
        
        if semantic_result.get('errors'):
            result['success'] = False
            result['errors'].extend(semantic_result['errors'])
        
        if semantic_result.get('warnings'):
            result['warnings'].extend(semantic_result['warnings'])
        
        # Execute code if all phases successful
        if result['success']:
            exec_result = self.execute_code(parser_result.get('ast'))
            result['output'] = exec_result.get('output', '')
            if exec_result.get('errors'):
                result['errors'].extend(exec_result['errors'])
        
        return result
    
    def lexical_analysis(self, code):
        """Phase 1: Tokenize the input code"""
        lexer = lex.lex()
        lexer.input(code)
        
        tokens_list = []
        errors = []
        
        try:
            for tok in lexer:
                tokens_list.append({
                    'type': tok.type,
                    'value': str(tok.value),
                    'line': tok.lineno,
                    'position': tok.lexpos
                })
        except Exception as e:
            errors.append(f"Lexical error: {str(e)}")
        
        return {
            'tokens': tokens_list,
            'token_count': len(tokens_list),
            'errors': errors
        }
    
    def syntax_analysis(self, code):
        """Phase 2: Parse and build AST"""
        self.errors = []
        
        # Grammar rules
        def p_program(p):
            '''program : statement_list'''
            p[0] = {'type': 'Program', 'body': p[1]}
        
        def p_statement_list(p):
            '''statement_list : statement_list statement
                              | statement'''
            if len(p) == 3:
                p[0] = p[1] + [p[2]]
            else:
                p[0] = [p[1]]
        
        def p_statement(p):
            '''statement : var_declaration
                        | function_declaration
                        | assignment
                        | if_statement
                        | while_statement
                        | for_statement
                        | return_statement
                        | print_statement
                        | expression SEMICOLON'''
            p[0] = p[1]
        
        def p_var_declaration(p):
            '''var_declaration : VAR IDENTIFIER COLON type SEMICOLON
                              | VAR IDENTIFIER COLON type ASSIGN expression SEMICOLON'''
            if len(p) == 6:
                p[0] = {'type': 'VarDeclaration', 'name': p[2], 'datatype': p[4], 'value': None}
            else:
                p[0] = {'type': 'VarDeclaration', 'name': p[2], 'datatype': p[4], 'value': p[6]}
        
        def p_type(p):
            '''type : INT
                   | FLOAT
                   | BOOL'''
            p[0] = p[1]
        
        def p_function_declaration(p):
            '''function_declaration : FUNC IDENTIFIER LPAREN param_list RPAREN COLON type LBRACE statement_list RBRACE
                                   | FUNC IDENTIFIER LPAREN RPAREN COLON type LBRACE statement_list RBRACE'''
            if len(p) == 11:
                p[0] = {'type': 'FunctionDeclaration', 'name': p[2], 'params': p[4], 'return_type': p[7], 'body': p[9]}
            else:
                p[0] = {'type': 'FunctionDeclaration', 'name': p[2], 'params': [], 'return_type': p[6], 'body': p[8]}
        
        def p_param_list(p):
            '''param_list : param_list COMMA param
                         | param'''
            if len(p) == 4:
                p[0] = p[1] + [p[3]]
            else:
                p[0] = [p[1]]
        
        def p_param(p):
            '''param : IDENTIFIER COLON type'''
            p[0] = {'name': p[1], 'type': p[3]}
        
        def p_assignment(p):
            '''assignment : IDENTIFIER ASSIGN expression SEMICOLON'''
            p[0] = {'type': 'Assignment', 'name': p[1], 'value': p[3]}
        
        def p_if_statement(p):
            '''if_statement : IF LPAREN expression RPAREN LBRACE statement_list RBRACE
                           | IF LPAREN expression RPAREN LBRACE statement_list RBRACE ELSE LBRACE statement_list RBRACE'''
            if len(p) == 8:
                p[0] = {'type': 'IfStatement', 'condition': p[3], 'then': p[6], 'else': None}
            else:
                p[0] = {'type': 'IfStatement', 'condition': p[3], 'then': p[6], 'else': p[10]}
        
        def p_while_statement(p):
            '''while_statement : WHILE LPAREN expression RPAREN LBRACE statement_list RBRACE'''
            p[0] = {'type': 'WhileStatement', 'condition': p[3], 'body': p[6]}
        
        def p_for_statement(p):
            '''for_statement : FOR LPAREN assignment expression SEMICOLON assignment RPAREN LBRACE statement_list RBRACE'''
            p[0] = {'type': 'ForStatement', 'init': p[3], 'condition': p[4], 'update': p[6], 'body': p[9]}
        
        def p_return_statement(p):
            '''return_statement : RETURN expression SEMICOLON'''
            p[0] = {'type': 'ReturnStatement', 'value': p[2]}
        
        def p_print_statement(p):
            '''print_statement : PRINT LPAREN expression RPAREN SEMICOLON'''
            p[0] = {'type': 'PrintStatement', 'value': p[3]}
        
        def p_expression_binop(p):
            '''expression : expression PLUS expression
                         | expression MINUS expression
                         | expression TIMES expression
                         | expression DIVIDE expression
                         | expression EQ expression
                         | expression NE expression
                         | expression LT expression
                         | expression LE expression
                         | expression GT expression
                         | expression GE expression
                         | expression AND expression
                         | expression OR expression'''
            p[0] = {'type': 'BinaryOp', 'op': p[2], 'left': p[1], 'right': p[3]}
        
        def p_expression_unary(p):
            '''expression : NOT expression
                         | MINUS expression %prec UMINUS'''
            p[0] = {'type': 'UnaryOp', 'op': p[1], 'operand': p[2]}
        
        def p_expression_group(p):
            '''expression : LPAREN expression RPAREN'''
            p[0] = p[2]
        
        def p_expression_function_call(p):
            '''expression : IDENTIFIER LPAREN arg_list RPAREN
                         | IDENTIFIER LPAREN RPAREN'''
            if len(p) == 5:
                p[0] = {'type': 'FunctionCall', 'name': p[1], 'args': p[3]}
            else:
                p[0] = {'type': 'FunctionCall', 'name': p[1], 'args': []}
        
        def p_arg_list(p):
            '''arg_list : arg_list COMMA expression
                       | expression'''
            if len(p) == 4:
                p[0] = p[1] + [p[3]]
            else:
                p[0] = [p[1]]
        
        def p_expression_identifier(p):
            '''expression : IDENTIFIER'''
            p[0] = {'type': 'Identifier', 'name': p[1]}
        
        def p_expression_number(p):
            '''expression : NUMBER'''
            p[0] = {'type': 'Literal', 'value': p[1], 'datatype': 'int'}
        
        def p_expression_float(p):
            '''expression : FLOATNUM'''
            p[0] = {'type': 'Literal', 'value': p[1], 'datatype': 'float'}
        
        def p_expression_bool(p):
            '''expression : TRUE
                         | FALSE'''
            p[0] = {'type': 'Literal', 'value': p[1] == 'true', 'datatype': 'bool'}
        
        def p_error(p):
            if p:
                self.errors.append(f"Syntax error at line {p.lineno}: Unexpected token '{p.value}'")
            else:
                self.errors.append("Syntax error: Unexpected end of input")
        
        # Build parser
        lexer = lex.lex()
        parser = yacc.yacc(debug=False, write_tables=False)
        
        try:
            ast = parser.parse(code, lexer=lexer)
            
            return {
                'ast': ast,
                'ast_json': json.dumps(ast, indent=2) if ast else None,
                'errors': self.errors if self.errors else []
            }
        except Exception as e:
            self.errors.append(f"Parse error: {str(e)}")
            return {
                'ast': None,
                'ast_json': None,
                'errors': self.errors
            }
    
    def semantic_analysis(self, ast):
        """Phase 3: Semantic checking"""
        self.errors = []
        self.warnings = []
        self.scoped_symbols = defaultdict(dict)
        self.function_table = {}
        self.current_scope = 'global'
        
        if not ast:
            return {'errors': ['No AST available for semantic analysis']}
        
        # Analyze the AST
        self.analyze_node(ast)
        
        # Build symbol table for display
        symbol_table_display = []
        for scope, symbols in self.scoped_symbols.items():
            for name, info in symbols.items():
                symbol_table_display.append({
                    'name': name,
                    'type': info.get('type', 'unknown'),
                    'scope': scope,
                    'kind': info.get('kind', 'variable'),
                    'initialized': info.get('initialized', False)
                })
        
        return {
            'symbol_table': symbol_table_display,
            'function_table': self.function_table,
            'errors': self.errors,
            'warnings': self.warnings
        }
    
    def analyze_node(self, node, expected_return_type=None):
        """Recursively analyze AST nodes"""
        if not node:
            return None
        
        node_type = node.get('type')
        
        if node_type == 'Program':
            for stmt in node.get('body', []):
                self.analyze_node(stmt)
        
        elif node_type == 'VarDeclaration':
            name = node.get('name')
            datatype = node.get('datatype')
            value = node.get('value')
            
            if name in self.scoped_symbols[self.current_scope]:
                self.errors.append(f"Variable '{name}' already declared in scope '{self.current_scope}'")
        
        elif node_type == 'FunctionCall':
            name = node.get('name')
            args = node.get('args', [])
            
            if name in env['global']:
                func = env['global'][name]
                params = func.get('params', [])
                body = func.get('body', [])
                
                func_env = dict(env['global'])
                for param, arg in zip(params, args):
                    func_env[param['name']] = execute_node(arg, local_env)
                
                for stmt in body:
                    result = execute_node(stmt, func_env)
                    if result is not None and isinstance(result, dict) and result.get('return'):
                        # Unwrap and return the actual value from the function
                        return result['value']
                
                return None
