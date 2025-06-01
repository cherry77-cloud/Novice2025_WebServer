#!/root/miniconda3/bin/python3
# -*- coding: utf-8 -*-

import os
import html
import urllib.parse
from datetime import datetime
import sys


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

def get_thai_recipe_advice(question):
    """使用简化的RAG获取Thai菜谱建议"""
    try:
        # 重定向所有输出到null避免任何打印
        original_stdout = sys.stdout
        original_stderr = sys.stderr
        
        with open(os.devnull, 'w') as devnull:
            sys.stdout = devnull
            sys.stderr = devnull
            
            try:
                from langchain_openai import ChatOpenAI, OpenAIEmbeddings
                from langchain_community.document_loaders import PyPDFLoader
                from langchain_community.vectorstores import FAISS
                from langchain.text_splitter import RecursiveCharacterTextSplitter
                from langchain.schema import Document
                
                # 恢复输出
                sys.stdout = original_stdout
                sys.stderr = original_stderr
                
                # 设置向量存储路径
                vector_store_path = "/root/autodl-tmp/WebServer-master-v1/resources/thai_recipes_index"
                pdf_path = "/root/autodl-tmp/WebServer-master-v1/resources/ThaiRecipes.pdf"
                
                # 初始化模型
                llm = ChatOpenAI(
                    model="gpt-4o-2024-08-06",
                    api_key="sk-Kcu4BKf2QEMNeFrgEN7CRL646tqv4WAraZFrpfAnxCz5KsoS",
                    base_url="https://xiaoai.plus/v1",
                    temperature=0.1
                )
                
                embeddings = OpenAIEmbeddings(
                    model="text-embedding-3-small",
                    api_key="sk-Kcu4BKf2QEMNeFrgEN7CRL646tqv4WAraZFrpfAnxCz5KsoS",
                    base_url="https://xiaoai.plus/v1"
                )
                
                # 检查是否已有向量索引
                if os.path.exists(vector_store_path):
                    # 加载现有索引
                    vectorstore = FAISS.load_local(vector_store_path, embeddings, allow_dangerous_deserialization=True)
                else:
                    # 创建新索引
                    if os.path.exists(pdf_path):
                        # 加载并分割PDF
                        loader = PyPDFLoader(pdf_path)
                        documents = loader.load()
                        
                        text_splitter = RecursiveCharacterTextSplitter(
                            chunk_size=1000,
                            chunk_overlap=200
                        )
                        texts = text_splitter.split_documents(documents)
                        
                        # 创建向量存储
                        vectorstore = FAISS.from_documents(texts, embeddings)
                        vectorstore.save_local(vector_store_path)
                    else:
                        # 如果没有PDF，使用默认知识
                        default_docs = [
                            Document(page_content="冬阴功汤是泰国最著名的酸辣汤，主要材料包括虾、柠檬草、青柠叶、辣椒、鱼露等。"),
                            Document(page_content="泰式绿咖喱使用椰浆、绿咖喱酱、罗勒叶等制作，口味浓郁香辣。"),
                            Document(page_content="椰浆鸡汤(Tom Kha Gai)是温和的泰式汤品，使用椰浆、南姜、柠檬草调味。")
                        ]
                        vectorstore = FAISS.from_documents(default_docs, embeddings)
                        vectorstore.save_local(vector_store_path)
                
                # 搜索相关文档
                relevant_docs = vectorstore.similarity_search(question, k=3)
                context = "\n".join([doc.page_content for doc in relevant_docs])
                
                # 生成回答
                prompt = f"""基于以下泰式料理知识回答问题，请用中文回答：

知识库内容：
{context}

问题：{question}

请提供详细的制作方法、所需材料和烹饪技巧。如果知识库中没有相关信息，请提供你了解的泰式料理常识。"""

                response = llm.invoke(prompt)
                return response.content
                
            except Exception as e:
                sys.stdout = original_stdout
                sys.stderr = original_stderr
                return "正在为您查找泰式料理信息..."
                
    except Exception as e:
        return "正在为您查找泰式料理信息..."

def show_page(question="", answer=""):
    """显示页面"""
    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    # 使用普通字符串避免格式化错误
    page_html = '''<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🍜 Thai菜谱专家</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #ff6b6b 0%, #ffa500 50%, #32cd32 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 900px;
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
            background: linear-gradient(135deg, #ff6b6b, #ffa500);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .form-section {
            margin-bottom: 25px;
        }
        .form-label {
            display: block;
            margin-bottom: 10px;
            color: #333;
            font-weight: 600;
            font-size: 1.1em;
        }
        .question-input {
            width: 100%;
            padding: 15px;
            border: 2px solid #e1e8ed;
            border-radius: 10px;
            font-size: 16px;
            outline: none;
        }
        .question-input:focus {
            border-color: #ff6b6b;
            box-shadow: 0 0 0 3px rgba(255, 107, 107, 0.1);
        }
        .submit-btn {
            background: linear-gradient(135deg, #ff6b6b, #ffa500);
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
            box-shadow: 0 8px 15px rgba(255, 107, 107, 0.3);
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
            border-left: 5px solid #ff6b6b;
        }
        .result-section h3 {
            color: #333;
            margin-bottom: 15px;
            font-size: 1.3em;
        }
        .answer-display {
            background: #f8fafc;
            padding: 20px;
            border-radius: 10px;
            line-height: 1.6;
            color: #374151;
            white-space: pre-wrap;
        }
        .example-section {
            margin-top: 20px;
            padding: 15px;
            background: #fff5f5;
            border-radius: 10px;
            border-left: 4px solid #ff6b6b;
        }
        .example-links {
            display: flex;
            gap: 15px;
            flex-wrap: wrap;
        }
        .example-link {
            color: #ff6b6b;
            cursor: pointer;
            font-weight: 500;
            text-decoration: underline;
        }
        .example-link:hover {
            color: #ffa500;
        }
        .back-link {
            text-align: center;
            margin-top: 20px;
        }
        .back-link a {
            color: #ff6b6b;
            text-decoration: none;
            font-weight: 500;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🍜 Thai菜谱专家</h1>
            <p style="color: #666;">基于RAG的专业泰式料理指导助手</p>
        </div>
        
        <form method="GET" action="/cgi-bin/thai-recipe-expert.cgi">
            <div class="form-section">
                <label class="form-label">🍛 请问您想了解什么泰式料理？</label>
                <input type="text" name="question" class="question-input" 
                       value="''' + html.escape(question) + '''"
                       placeholder="例如：如何制作冬阴功汤？泰式绿咖喱的做法？" 
                       required>
            </div>
            
            <button type="submit" class="submit-btn">🔍 咨询专家</button>
            <button type="button" class="clear-btn" onclick="document.querySelector('input').value=''">🗑️ 清空</button>
        </form>'''

    if answer:
        page_html += '''
        <div class="result-section">
            <h3>👨‍🍳 专家建议</h3>
            <div class="answer-display">''' + html.escape(answer) + '''</div>
            <div class="back-link">
                <small style="color: #6b7280;">回答时间: ''' + current_time + '''</small><br>
                <a href="/cgi-bin/thai-recipe-expert.cgi">🔄 咨询新问题</a>
            </div>
        </div>'''

    page_html += '''
        <div class="example-section">
            <p style="margin-bottom: 10px; font-weight: 600;">🍜 热门问题示例:</p>
            <div class="example-links">
                <span class="example-link" onclick="askQuestion('如何制作正宗的冬阴功汤？')">冬阴功汤制作</span>
                <span class="example-link" onclick="askQuestion('泰式绿咖喱的详细做法')">泰式绿咖喱</span>
                <span class="example-link" onclick="askQuestion('椰浆鸡汤怎么做？')">椰浆鸡汤</span>
                <span class="example-link" onclick="askQuestion('泰式炒河粉的秘诀')">泰式炒河粉</span>
                <span class="example-link" onclick="askQuestion('芒果糯米饭的制作方法')">芒果糯米饭</span>
                <span class="example-link" onclick="askQuestion('泰式咖喱的历史起源')">泰式咖喱历史</span>
            </div>
        </div>
        
        <div style="margin-top: 20px; padding: 15px; background: #f0f9ff; border-radius: 10px; text-align: center;">
            <p style="color: #0369a1; font-weight: 500;">💡 基于PDF知识库的RAG检索增强生成，提供准确的泰式料理指导</p>
        </div>
    </div>
    
    <script>
        function askQuestion(question) {
            document.querySelector('input[name="question"]').value = question;
        }
    </script>
</body>
</html>'''
    
    print(page_html)

def main():
    print_headers()
    
    params = get_query_params()
    question = params.get("question", "").strip()
    answer = ""
    
    if question:
        answer = get_thai_recipe_advice(question)
    
    show_page(question, answer)

if __name__ == "__main__":
    main() 