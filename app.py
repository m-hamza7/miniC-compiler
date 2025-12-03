from flask import Flask, render_template, request, jsonify
import sys
import io
import subprocess
import json
from minic_compiler_new import MiniCCompiler

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/compile', methods=['POST'])
def compile_code():
    """Handle code compilation and return results from all phases"""
    try:
        code = request.json.get('code', '')
        
        if not code.strip():
            return jsonify({
                'success': False,
                'error': 'No code provided'
            })

        # If C++ backend executable exists, call it via subprocess
        exe_path = r'backend_cpp\\minic_backend.exe'
        try:
            # Run the C++ backend, send code via stdin, expect JSON on stdout
            proc = subprocess.run([exe_path], input=code.encode('utf-8'), stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
            out = proc.stdout.decode('utf-8')
            # Attempt to parse JSON
            result = json.loads(out)
            return jsonify(result)
        except FileNotFoundError:
            # Fall back to Python compiler if executable not found
            compiler = MiniCCompiler()
            result = compiler.compile(code)
            return jsonify(result)
        except subprocess.CalledProcessError as e:
            # If C++ process failed, return error and stderr
            stderr = e.stderr.decode('utf-8') if hasattr(e, 'stderr') and e.stderr else str(e)
            return jsonify({
                'success': False,
                'error': 'C++ backend error',
                'details': stderr
            })
        except Exception as e:
            return jsonify({
                'success': False,
                'error': 'Failed to run backend',
                'details': str(e)
            })
        
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        })

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
