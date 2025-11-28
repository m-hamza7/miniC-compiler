from flask import Flask, render_template, request, jsonify
import sys
import io
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
        
        # Create compiler instance
        compiler = MiniCCompiler()
        
        # Run all compiler phases
        result = compiler.compile(code)
        
        return jsonify(result)
        
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        })

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
