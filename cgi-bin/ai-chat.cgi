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

def chat_with_ai(user_message):
    """使用AI进行对话"""
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
                    description="You are a helpful AI assistant that provides informative and friendly responses.",
                    instructions=[
                        "Always respond in Chinese",
                        "Be helpful, informative, and friendly",
                        "Provide practical and useful information",
                        "If you don't know something, be honest about it",
                        "Keep responses well-formatted and easy to read",
                    ],
                    markdown=True,
                )
                
                # 执行对话
                response = agent.run(user_message)
                return response.content if hasattr(response, 'content') else str(response)
                
            except Exception as e:
                sys.stdout = original_stdout
                sys.stderr = original_stderr
                return f"正在为您处理问题，请稍候..."
                
    except Exception as e:
        return f"正在为您处理问题，请稍候..."

def show_page(user_message="", ai_response=""):
    """显示页面"""
    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    page_html = '''<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🤖 AI聊天助手</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #6366f1 0%, #8b5cf6 50%, #ec4899 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1000px;
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
            background: linear-gradient(135deg, #6366f1, #8b5cf6);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .chat-section {
            margin-bottom: 25px;
        }
        .chat-label {
            display: block;
            margin-bottom: 10px;
            color: #333;
            font-weight: 600;
            font-size: 1.1em;
        }
        .chat-input {
            width: 100%;
            padding: 15px;
            border: 2px solid #e1e8ed;
            border-radius: 10px;
            font-size: 16px;
            outline: none;
            min-height: 100px;
            resize: vertical;
            font-family: inherit;
        }
        .chat-input:focus {
            border-color: #6366f1;
            box-shadow: 0 0 0 3px rgba(99, 102, 241, 0.1);
        }
        .submit-btn {
            background: linear-gradient(135deg, #6366f1, #8b5cf6);
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
            box-shadow: 0 8px 15px rgba(99, 102, 241, 0.3);
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
            border-left: 5px solid #6366f1;
        }
        .result-section h3 {
            color: #333;
            margin-bottom: 15px;
            font-size: 1.3em;
        }
        .chat-display {
            background: #f8fafc;
            padding: 20px;
            border-radius: 10px;
            line-height: 1.6;
            color: #374151;
            white-space: pre-wrap;
            max-height: 600px;
            overflow-y: auto;
        }
        .user-message {
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
            border-left: 4px solid #6366f1;
        }
        .example-links {
            display: flex;
            gap: 15px;
            flex-wrap: wrap;
        }
        .example-link {
            color: #6366f1;
            cursor: pointer;
            font-weight: 500;
            text-decoration: underline;
            padding: 5px 10px;
            border-radius: 5px;
            background: rgba(99, 102, 241, 0.1);
        }
        .example-link:hover {
            color: #8b5cf6;
            background: rgba(139, 92, 246, 0.1);
        }
        .back-link {
            text-align: center;
            margin-top: 20px;
        }
        .back-link a {
            color: #6366f1;
            text-decoration: none;
            font-weight: 500;
        }
        .info-box {
            margin-top: 20px;
            padding: 15px;
            background: #f0f9ff;
            border: 1px solid #6366f1;
            border-radius: 10px;
            color: #1e40af;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🤖 AI聊天助手</h1>
            <p style="color: #666;">基于GPT-4o的智能对话助手，可以回答各种问题</p>
        </div>
        
        <form method="GET" action="/cgi-bin/ai-chat.cgi">
            <div class="chat-section">
                <label class="chat-label">💬 请输入您的问题或话题</label>
                <textarea name="message" class="chat-input" 
                          placeholder="例如：解释什么是人工智能、推荐一些学习Python的方法、介绍一下量子计算..." 
                          required>''' + html.escape(user_message) + '''</textarea>
            </div>
            
            <button type="submit" class="submit-btn">🚀 发送消息</button>
            <button type="button" class="clear-btn" onclick="document.querySelector('textarea').value=''">🗑️ 清空</button>
        </form>'''

    if ai_response and user_message:
        page_html += '''
        <div class="result-section">
            <h3>💭 对话记录</h3>
            <div class="user-message">
                <strong>👤 您的问题:</strong><br>
                ''' + html.escape(user_message) + '''
            </div>
            <div class="chat-display">
                <strong>🤖 AI助手回复:</strong><br><br>
                ''' + html.escape(ai_response) + '''
            </div>
            <div class="back-link">
                <small style="color: #6b7280;">对话时间: ''' + current_time + '''</small><br>
                <a href="/cgi-bin/ai-chat.cgi">💬 新的对话</a>
            </div>
        </div>'''

    page_html += '''
        <div class="example-section">
            <p style="margin-bottom: 10px; font-weight: 600;">💡 话题示例:</p>
            <div class="example-links">
                <span class="example-link" onclick="askQuestion('解释什么是人工智能')">解释人工智能</span>
                <span class="example-link" onclick="askQuestion('推荐学习Python的方法')">Python学习方法</span>
                <span class="example-link" onclick="askQuestion('介绍量子计算原理')">量子计算原理</span>
                <span class="example-link" onclick="askQuestion('健康饮食的建议')">健康饮食建议</span>
                <span class="example-link" onclick="askQuestion('如何提高工作效率')">提高工作效率</span>
                <span class="example-link" onclick="askQuestion('推荐一些好书')">好书推荐</span>
            </div>
        </div>
        
        <div class="info-box">
            <p><strong>💡 使用说明：</strong> AI助手可以回答各种问题，提供信息、建议和解释。请随意提问！</p>
        </div>
        
        <div style="margin-top: 20px; padding: 15px; background: #f0f9ff; border-radius: 10px; text-align: center;">
            <p style="color: #1e40af; font-weight: 500;">🌟 基于GPT-4o的智能对话，让AI成为您的知识伙伴</p>
        </div>
    </div>
    
    <script>
        function askQuestion(question) {
            document.querySelector('textarea[name="message"]').value = question;
        }
    </script>
</body>
</html>'''
    
    print(page_html)

def main():
    print_headers()
    
    params = get_query_params()
    user_message = params.get("message", "").strip()
    ai_response = ""
    
    if user_message:
        ai_response = chat_with_ai(user_message)
    
    show_page(user_message, ai_response)

if __name__ == "__main__":
    main() 