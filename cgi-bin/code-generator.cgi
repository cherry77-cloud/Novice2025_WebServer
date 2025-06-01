#!/root/miniconda3/bin/python3
# -*- coding: utf-8 -*-

import os
import html
import urllib.parse
from datetime import datetime
import sys
import logging

# 禁用所有日志输出
logging.disable(logging.CRITICAL)

def print_headers():
    """打印HTTP头"""
    print("Content-Type: text/html; charset=utf-8")
    print()

def get_query_params():
    """获取GET请求参数"""
    query_string = os.environ.get('QUERY_STRING', '')
    params = {}
    if query_string:
        for param in query_string.split('&'):
            if '=' in param:
                key, value = param.split('=', 1)
                params[urllib.parse.unquote(key)] = urllib.parse.unquote_plus(value)
    return params

def generate_code_with_ai(code_request):
    """使用AI生成代码"""
    try:
        # 重定向输出避免日志干扰
        original_stdout = sys.stdout
        original_stderr = sys.stderr
        
        with open(os.devnull, 'w') as devnull:
            sys.stdout = devnull
            sys.stderr = devnull
            
            try:
                from agno.agent import Agent
                from agno.models.openai import OpenAIChat
                
                # 恢复输出
                sys.stdout = original_stdout
                sys.stderr = original_stderr
                
                agent = Agent(
                    model=OpenAIChat(
                        id="gpt-4o-mini",
                        api_key="sk-Kcu4BKf2QEMNeFrgEN7CRL646tqv4WAraZFrpfAnxCz5KsoS",
                        base_url="https://xiaoai.plus/v1"
                    ),
                    description="You are a professional code generation assistant that creates high-quality Python code.",
                    instructions=[
                        "Generate complete, working Python code based on user requirements",
                        "Always respond in Chinese",
                        "Include detailed code comments and explanations",
                        "Provide usage examples and test cases",
                        "Consider error handling and best practices",
                        "Format code properly with proper indentation",
                        "Explain the logic and architecture of the code",
                        "Suggest improvements and optimizations",
                    ],
                    markdown=True,
                )
                
                prompt = f"""
请为以下需求生成完整的Python代码实现：

需求描述: {code_request}

请提供:
1. 完整的可运行代码
2. 详细的代码说明和注释
3. 使用示例和测试代码
4. 依赖库说明
5. 注意事项和最佳实践

代码要求:
- 遵循PEP 8编码规范
- 包含适当的错误处理
- 代码结构清晰易懂
- 包含必要的文档字符串
- 提供完整的实现方案
"""
                
                response = agent.run(prompt)
                return response.content if hasattr(response, 'content') else str(response)
                
            except Exception as e:
                sys.stdout = original_stdout
                sys.stderr = original_stderr
                return f"正在为您生成 '{code_request}' 的代码实现，请稍候..."
                
    except Exception as e:
        return f"正在为您生成 '{code_request}' 的代码实现，请稍候..."

def show_page(code_request="", generated_code=""):
    """显示页面"""
    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    page_html = '''<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🔧 AI代码生成器</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 50%, #f093fb 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.95);
            backdrop-filter: blur(20px);
            border-radius: 20px;
            padding: 40px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        .header h1 {
            color: #333;
            font-size: 2.5em;
            margin-bottom: 10px;
            background: linear-gradient(135deg, #667eea, #764ba2);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .code-section {
            margin-bottom: 25px;
        }
        .code-label {
            display: block;
            margin-bottom: 10px;
            color: #333;
            font-weight: 600;
            font-size: 1.1em;
        }
        .code-input {
            width: 100%;
            padding: 15px;
            border: 2px solid #e1e8ed;
            border-radius: 10px;
            font-size: 16px;
            outline: none;
            min-height: 120px;
            resize: vertical;
            font-family: inherit;
        }
        .code-input:focus {
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        .submit-btn {
            background: linear-gradient(135deg, #667eea, #764ba2);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 8px;
            font-size: 1.1em;
            font-weight: 600;
            cursor: pointer;
            margin-top: 15px;
            margin-right: 10px;
        }
        .submit-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 15px rgba(102, 126, 234, 0.3);
        }
        .clear-btn {
            background: #6b7280;
            color: white;
            border: none;
            padding: 12px 20px;
            border-radius: 8px;
            cursor: pointer;
            margin-top: 15px;
        }
        .result-section {
            margin-top: 30px;
            padding: 25px;
            background: white;
            border-radius: 15px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.05);
            border-left: 5px solid #667eea;
        }
        .result-section h3 {
            color: #333;
            margin-bottom: 15px;
            font-size: 1.3em;
        }
        .code-display {
            background: #1e1e1e;
            color: #f8f8f2;
            padding: 20px;
            border-radius: 10px;
            line-height: 1.6;
            white-space: pre-wrap;
            max-height: 600px;
            overflow-y: auto;
            font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
            font-size: 14px;
        }
        .user-request {
            background: #e0f2fe;
            border-left: 4px solid #0288d1;
            padding: 15px;
            margin-bottom: 15px;
            border-radius: 10px;
        }
        .example-section {
            margin-top: 20px;
            padding: 15px;
            background: #f3f4f6;
            border-radius: 10px;
            border-left: 4px solid #667eea;
        }
        .example-links {
            display: flex;
            gap: 15px;
            flex-wrap: wrap;
        }
        .example-link {
            color: #667eea;
            cursor: pointer;
            font-weight: 500;
            text-decoration: underline;
            padding: 5px 10px;
            border-radius: 5px;
            background: rgba(102, 126, 234, 0.1);
        }
        .example-link:hover {
            color: #764ba2;
            background: rgba(118, 75, 162, 0.1);
        }
        .back-link {
            text-align: center;
            margin-top: 20px;
        }
        .back-link a {
            color: #667eea;
            text-decoration: none;
            font-weight: 500;
        }
        .info-box {
            margin-top: 20px;
            padding: 15px;
            background: #f0f9ff;
            border: 1px solid #667eea;
            border-radius: 10px;
            color: #1e40af;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔧 AI代码生成器</h1>
            <p style="color: #666;">基于GPT-4o的专业代码生成工具</p>
        </div>
        
        <form method="GET" action="/cgi-bin/code-generator.cgi">
            <div class="code-section">
                <label class="code-label">💻 请描述您的代码需求</label>
                <textarea name="query" class="code-input" 
                          placeholder="例如：编写一个文件上传功能、实现一个简单的网络爬虫、创建一个数据分析脚本..." 
                          required>''' + html.escape(code_request) + '''</textarea>
            </div>
            
            <button type="submit" class="submit-btn">🚀 生成代码</button>
            <button type="button" class="clear-btn" onclick="document.querySelector('textarea').value=''">🗑️ 清空</button>
        </form>'''

    if generated_code and code_request:
        page_html += '''
        <div class="result-section">
            <h3>🤖 AI生成的代码</h3>
            <div class="user-request">
                <strong>📝 您的需求:</strong><br>
                ''' + html.escape(code_request) + '''
            </div>
            <div class="code-display">''' + html.escape(generated_code) + '''</div>
            <div class="back-link">
                <small style="color: #6b7280;">生成时间: ''' + current_time + '''</small><br>
                <a href="/cgi-bin/code-generator.cgi">🔧 生成新代码</a>
            </div>
        </div>'''

    page_html += '''
        <div class="example-section">
            <p style="margin-bottom: 10px; font-weight: 600;">💡 代码需求示例:</p>
            <div class="example-links">
                <span class="example-link" onclick="generateCode('文件上传功能')">文件上传功能</span>
                <span class="example-link" onclick="generateCode('网络爬虫脚本')">网络爬虫脚本</span>
                <span class="example-link" onclick="generateCode('数据分析工具')">数据分析工具</span>
                <span class="example-link" onclick="generateCode('API接口设计')">API接口设计</span>
                <span class="example-link" onclick="generateCode('数据库操作类')">数据库操作类</span>
                <span class="example-link" onclick="generateCode('自动化脚本')">自动化脚本</span>
            </div>
        </div>
        
        <div class="info-box">
            <p><strong>💡 使用说明：</strong> 描述您的代码需求，AI将为您生成高质量的Python代码实现，包含注释和使用示例</p>
        </div>
        
        <div style="margin-top: 20px; padding: 15px; background: #f0f9ff; border-radius: 10px; text-align: center;">
            <p style="color: #1e40af; font-weight: 500;">🌟 基于AI的智能代码生成，让编程更高效</p>
        </div>
    </div>
    
    <script>
        function generateCode(requirement) {
            document.querySelector('textarea[name="query"]').value = requirement;
        }
    </script>
</body>
</html>'''
    
    print(page_html)

def main():
    print_headers()
    
    params = get_query_params()
    code_request = params.get("query", "").strip()
    generated_code = ""
    
    if code_request:
        generated_code = generate_code_with_ai(code_request)
    
    show_page(code_request, generated_code)

if __name__ == "__main__":
    main() 