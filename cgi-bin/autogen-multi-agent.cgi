#!/root/miniconda3/bin/python3
# -*- coding: utf-8 -*-

import asyncio
import os
import html
from typing import List
from datetime import datetime

from autogen_core import (
    DefaultTopicId,
    MessageContext,
    RoutedAgent,
    SingleThreadedAgentRuntime,
    TopicId,
    TypeSubscription,
    message_handler,
)
from autogen_core.models import (
    AssistantMessage,
    ChatCompletionClient,
    LLMMessage,
    SystemMessage,
    UserMessage,
)
from autogen_ext.models.openai import OpenAIChatCompletionClient
from pydantic import BaseModel

def print_headers():
    """打印HTTP头"""
    print("Content-Type: text/html; charset=utf-8")
    print("Cache-Control: no-cache")
    print("")

def get_query_params():
    """获取GET查询参数"""
    params = {}
    query_string = os.environ.get('QUERY_STRING', '')
    if query_string:
        import urllib.parse
        for param in query_string.split('&'):
            if '=' in param:
                key, value = param.split('=', 1)
                params[urllib.parse.unquote(key)] = urllib.parse.unquote(value)
    return params

def get_form_data():
    """获取POST表单数据"""
    try:
        import cgi
        form = cgi.FieldStorage()
        form_data = {}
        for key in form.keys():
            value = form.getvalue(key)
            if value:
                form_data[key] = value
        return form_data
    except:
        return {}

# Global variables for collecting output
conversation_output = []
final_story = ""

class GroupChatMessage(BaseModel):
    body: UserMessage

class RequestToSpeak(BaseModel):
    pass

class BaseGroupChatAgent(RoutedAgent):
    """A group chat participant using an LLM."""

    def __init__(
        self,
        description: str,
        group_chat_topic_type: str,
        model_client: ChatCompletionClient,
        system_message: str,
    ) -> None:
        super().__init__(description=description)
        self._group_chat_topic_type = group_chat_topic_type
        self._model_client = model_client
        self._system_message = SystemMessage(content=system_message)
        self._chat_history: List[LLMMessage] = []

    @message_handler
    async def handle_message(self, message: GroupChatMessage, ctx: MessageContext) -> None:
        self._chat_history.extend(
            [
                UserMessage(content=f"Transferred to {message.body.source}", source="system"),
                message.body,
            ]
        )

    @message_handler
    async def handle_request_to_speak(self, message: RequestToSpeak, ctx: MessageContext) -> None:
        global conversation_output, final_story
        
        # Add system message
        self._chat_history.append(
            UserMessage(content=f"Transferred to {self.id.type}, adopt the persona immediately.", source="system")
        )
        completion = await self._model_client.create([self._system_message] + self._chat_history)
        assert isinstance(completion.content, str)
        self._chat_history.append(AssistantMessage(content=completion.content, source=self.id.type))
        
        # Store output for CGI display
        conversation_output.append({
            'agent': self.id.type,
            'content': completion.content
        })
        
        # If this is the Reviewer, store as final story
        if self.id.type == "Reviewer":
            final_story = completion.content
        
        await self.publish_message(
            GroupChatMessage(body=UserMessage(content=completion.content, source=self.id.type)),
            topic_id=DefaultTopicId(type=self._group_chat_topic_type),
        )

class EditorAgent(BaseGroupChatAgent):
    def __init__(self, description: str, group_chat_topic_type: str, model_client: ChatCompletionClient) -> None:
        super().__init__(
            description=description,
            group_chat_topic_type=group_chat_topic_type,
            model_client=model_client,
            system_message="你是专业故事编辑。你的任务是为给定的主题制定详细的故事大纲，包括：1)故事背景和设定 2)主要人物和性格特点 3)故事结构（开头、发展、高潮、结局）4)主题和寓意。请提供简洁明确的创作指导。",
        )

class WriterAgent(BaseGroupChatAgent):
    def __init__(self, description: str, group_chat_topic_type: str, model_client: ChatCompletionClient) -> None:
        super().__init__(
            description=description,
            group_chat_topic_type=group_chat_topic_type,
            model_client=model_client,
            system_message="你是专业故事作家。根据编辑提供的大纲，创作一个完整的故事。要求：1)情节生动有趣 2)人物刻画鲜明 3)语言优美流畅 4)符合大纲要求 5)有完整的故事结构。请写出完整的故事内容。",
        )

class ReviewerAgent(BaseGroupChatAgent):
    def __init__(self, description: str, group_chat_topic_type: str, model_client: ChatCompletionClient) -> None:
        super().__init__(
            description=description,
            group_chat_topic_type=group_chat_topic_type,
            model_client=model_client,
            system_message="你是专业故事审核员。你的任务是审查作家创作的故事，并输出最终完善版本。要求：1)检查故事完整性和逻辑性 2)优化语言表达 3)确保故事质量 4)输出最终定稿版本。请提供经过审核和优化的最终故事版本。",
        )

class UserAgent(RoutedAgent):
    def __init__(self, description: str, group_chat_topic_type: str) -> None:
        super().__init__(description=description)
        self._group_chat_topic_type = group_chat_topic_type

    @message_handler
    async def handle_message(self, message: GroupChatMessage, ctx: MessageContext) -> None:
        pass

    @message_handler
    async def handle_request_to_speak(self, message: RequestToSpeak, ctx: MessageContext) -> None:
        # Auto-approve for CGI environment
        user_input = "APPROVE"
        await self.publish_message(
            GroupChatMessage(body=UserMessage(content=user_input, source=self.id.type)),
            DefaultTopicId(type=self._group_chat_topic_type),
        )

class GroupChatManager(RoutedAgent):
    def __init__(self, participant_topic_types: List[str]) -> None:
        super().__init__("Group chat manager")
        self._participant_topic_types = participant_topic_types
        self._chat_history: List[UserMessage] = []
        self._previous_participant_topic_type: str | None = None

    @message_handler
    async def handle_message(self, message: GroupChatMessage, ctx: MessageContext) -> None:
        assert isinstance(message.body, UserMessage)
        self._chat_history.append(message.body)
        
        # If the message is an approval message from the user, stop the chat.
        if message.body.source == "User":
            assert isinstance(message.body.content, str)
            if message.body.content.lower().strip().endswith("approve"):
                return
        
        # Simple sequential workflow: Editor -> Writer -> Reviewer
        if message.body.source == "User":
            selected_topic_type = "Editor"
        elif message.body.source == "Editor":
            selected_topic_type = "Writer"
        elif message.body.source == "Writer":
            selected_topic_type = "Reviewer"
        else:
            return
        
        self._previous_participant_topic_type = selected_topic_type
        await self.publish_message(RequestToSpeak(), DefaultTopicId(type=selected_topic_type))

async def run_autogen_multiagent(user_request: str):
    """运行AutoGen多智能体系统"""
    global conversation_output, final_story
    conversation_output = []
    final_story = ""
    
    runtime = SingleThreadedAgentRuntime()

    # 配置API
    API_KEY = "sk-Kcu4BKf2QEMNeFrgEN7CRL646tqv4WAraZFrpfAnxCz5KsoS"
    BASE_URL = "https://xiaoai.plus/v1"

    model_client = OpenAIChatCompletionClient(
        model="gpt-4o-2024-08-06",
        api_key=API_KEY,
        base_url=BASE_URL,
    )

    # Register agents
    editor_agent_type = await EditorAgent.register(
        runtime, "Editor",
        lambda: EditorAgent("编辑", "group_chat", model_client),
    )
    await runtime.add_subscription(TypeSubscription(topic_type="Editor", agent_type=editor_agent_type.type))
    await runtime.add_subscription(TypeSubscription(topic_type="group_chat", agent_type=editor_agent_type.type))

    writer_agent_type = await WriterAgent.register(
        runtime, "Writer",
        lambda: WriterAgent("写作", "group_chat", model_client),
    )
    await runtime.add_subscription(TypeSubscription(topic_type="Writer", agent_type=writer_agent_type.type))
    await runtime.add_subscription(TypeSubscription(topic_type="group_chat", agent_type=writer_agent_type.type))

    reviewer_agent_type = await ReviewerAgent.register(
        runtime, "Reviewer",
        lambda: ReviewerAgent("审核", "group_chat", model_client),
    )
    await runtime.add_subscription(TypeSubscription(topic_type="Reviewer", agent_type=reviewer_agent_type.type))
    await runtime.add_subscription(TypeSubscription(topic_type="group_chat", agent_type=reviewer_agent_type.type))

    user_agent_type = await UserAgent.register(
        runtime, "User",
        lambda: UserAgent("用户", "group_chat"),
    )
    await runtime.add_subscription(TypeSubscription(topic_type="User", agent_type=user_agent_type.type))
    await runtime.add_subscription(TypeSubscription(topic_type="group_chat", agent_type=user_agent_type.type))

    group_chat_manager_type = await GroupChatManager.register(
        runtime, "group_chat_manager",
        lambda: GroupChatManager(["Editor", "Writer", "Reviewer", "User"]),
    )
    await runtime.add_subscription(
        TypeSubscription(topic_type="group_chat", agent_type=group_chat_manager_type.type)
    )

    runtime.start()
    await runtime.publish_message(
        GroupChatMessage(body=UserMessage(content=user_request, source="User")),
        TopicId(type="group_chat", source="session"),
    )
    await runtime.stop_when_idle()
    await model_client.close()
    
    return {
        'conversation': conversation_output,
        'final_story': final_story
    }

def show_input_form():
    """显示用户输入表单"""
    print(f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AutoGen故事创作系统</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }}
        
        .form-container {{
            background: rgba(255,255,255,0.1);
            backdrop-filter: blur(20px);
            border-radius: 25px;
            padding: 40px;
            border: 1px solid rgba(255,255,255,0.2);
            box-shadow: 0 20px 40px rgba(0,0,0,0.3);
            max-width: 600px;
            width: 100%;
            text-align: center;
            color: white;
        }}
        
        .header {{
            margin-bottom: 30px;
        }}
        
        .header h1 {{
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
            background: linear-gradient(45deg, #FFD700, #FFA500);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }}
        
        .header p {{
            font-size: 1.2em;
            opacity: 0.9;
            margin-bottom: 20px;
        }}
        
        .workflow {{
            display: flex;
            justify-content: center;
            align-items: center;
            margin: 20px 0;
            font-size: 1em;
            flex-wrap: wrap;
        }}
        
        .agent-badge {{
            background: linear-gradient(45deg, #FF6B6B, #4ECDC4);
            padding: 8px 16px;
            border-radius: 20px;
            margin: 5px;
            font-weight: bold;
            font-size: 0.9em;
        }}
        
        .arrow {{
            margin: 0 10px;
            color: #FFD700;
            font-size: 1.2em;
        }}
        
        .form-group {{
            margin: 25px 0;
            text-align: left;
        }}
        
        .form-group label {{
            display: block;
            margin-bottom: 8px;
            font-weight: bold;
            font-size: 1.1em;
        }}
        
        .form-group input {{
            width: 100%;
            padding: 15px;
            border: none;
            border-radius: 15px;
            background: rgba(255,255,255,0.1);
            color: white;
            font-size: 1em;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
        }}
        
        .form-group input::placeholder {{
            color: rgba(255,255,255,0.7);
        }}
        
        .form-group input:focus {{
            outline: none;
            border-color: #FFD700;
            box-shadow: 0 0 15px rgba(255,215,0,0.3);
        }}
        
        .submit-btn {{
            background: linear-gradient(45deg, #FFD700, #FFA500);
            color: #333;
            padding: 15px 40px;
            border: none;
            border-radius: 25px;
            font-size: 1.2em;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 10px 20px rgba(0,0,0,0.2);
        }}
        
        .submit-btn:hover {{
            transform: translateY(-2px);
            box-shadow: 0 15px 30px rgba(0,0,0,0.3);
        }}
        
        .examples {{
            margin-top: 20px;
            text-align: left;
        }}
        
        .examples h3 {{
            margin-bottom: 10px;
            color: #FFD700;
        }}
        
        .example-item {{
            background: rgba(255,255,255,0.05);
            padding: 8px 12px;
            margin: 5px 0;
            border-radius: 10px;
            cursor: pointer;
            transition: all 0.2s ease;
            font-size: 0.9em;
        }}
        
        .example-item:hover {{
            background: rgba(255,255,255,0.1);
            transform: translateX(5px);
        }}
    </style>
</head>
<body>
    <div class="form-container">
        <div class="header">
            <h1>🤖 AutoGen故事创作</h1>
            <p>三智能体协作创作系统</p>
            <div class="workflow">
                <span class="agent-badge">📝 编辑</span>
                <span class="arrow">→</span>
                <span class="agent-badge">✍️ 写作</span>
                <span class="arrow">→</span>
                <span class="agent-badge">🔍 审核</span>
            </div>
        </div>
        
        <form method="GET" action="">
            <div class="form-group">
                <label for="story_request">请输入您想要的故事主题或要求：</label>
                <input type="text" id="story_request" name="prompt" placeholder="例如：写一个关于勇敢小老鼠的冒险故事..." required style="width: 100%; padding: 15px; border: none; border-radius: 15px; background: rgba(255,255,255,0.1); color: white; font-size: 1em; backdrop-filter: blur(10px); border: 1px solid rgba(255,255,255,0.2);">
            </div>
            
            <button type="submit" class="submit-btn">🚀 开始创作</button>
        </form>
        
        <div class="examples">
            <h3>💡 创作示例：</h3>
            <div class="example-item" onclick="fillExample('写一个关于友谊和勇气的童话故事')">
                🌟 友谊与勇气的童话
            </div>
            <div class="example-item" onclick="fillExample('创作一个发生在未来世界的科幻故事')">
                🚀 未来世界科幻冒险
            </div>
            <div class="example-item" onclick="fillExample('写一个温馨感人的家庭故事')">
                ❤️ 温馨家庭故事
            </div>
            <div class="example-item" onclick="fillExample('创作一个悬疑推理小故事')">
                🔍 悬疑推理故事
            </div>
        </div>
    </div>
    
    <script>
        function fillExample(text) {{
            document.getElementById('story_request').value = text;
        }}
    </script>
</body>
</html>""")

def show_result_page(user_request, result):
    """显示结果页面"""
    print(f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AutoGen故事创作结果</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            color: white;
            padding: 20px;
        }}
        
        .container {{
            max-width: 1200px;
            margin: 0 auto;
        }}
        
        .header {{
            text-align: center;
            margin-bottom: 30px;
        }}
        
        .header h1 {{
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }}
        
        .request-info {{
            background: rgba(255,255,255,0.1);
            padding: 20px;
            border-radius: 15px;
            margin-bottom: 30px;
            text-align: center;
            border: 1px solid rgba(255,255,255,0.2);
        }}
        
        .agent-section {{
            background: rgba(255,255,255,0.1);
            backdrop-filter: blur(20px);
            border-radius: 20px;
            padding: 25px;
            margin: 20px 0;
            border: 1px solid rgba(255,255,255,0.2);
            animation: slideInUp 0.6s ease forwards;
        }}
        
        @keyframes slideInUp {{
            from {{
                opacity: 0;
                transform: translateY(30px);
            }}
            to {{
                opacity: 1;
                transform: translateY(0);
            }}
        }}
        
        .agent-header {{
            display: flex;
            align-items: center;
            margin-bottom: 15px;
            font-size: 1.3em;
            font-weight: bold;
        }}
        
        .agent-icon {{
            font-size: 1.5em;
            margin-right: 10px;
        }}
        
        .agent-content {{
            line-height: 1.8;
            font-size: 1.1em;
            white-space: pre-wrap;
        }}
        
        .editor-section {{
            border-left: 4px solid #FFD700;
            background: linear-gradient(45deg, rgba(255,215,0,0.15), rgba(255,165,0,0.1));
        }}
        
        .writer-section {{
            border-left: 4px solid #4ECDC4;
            background: linear-gradient(45deg, rgba(78,205,196,0.15), rgba(72,179,189,0.1));
        }}
        
        .reviewer-section {{
            border-left: 4px solid #9B59B6;
            background: linear-gradient(45deg, rgba(155,89,182,0.15), rgba(142,68,173,0.1));
        }}
        
        .final-story {{
            background: linear-gradient(45deg, rgba(255,215,0,0.2), rgba(255,165,0,0.15));
            border: 2px solid #FFD700;
            border-radius: 25px;
            padding: 30px;
            margin: 30px 0;
            text-align: center;
        }}
        
        .final-story h2 {{
            color: #FFD700;
            margin-bottom: 20px;
            font-size: 2em;
        }}
        
        .final-story-content {{
            font-size: 1.2em;
            line-height: 2;
            text-align: left;
            white-space: pre-wrap;
        }}
        
        .back-btn {{
            display: inline-block;
            background: linear-gradient(45deg, #4ECDC4, #44A08D);
            color: white;
            padding: 12px 30px;
            text-decoration: none;
            border-radius: 25px;
            font-weight: bold;
            margin-top: 20px;
            transition: all 0.3s ease;
        }}
        
        .back-btn:hover {{
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(0,0,0,0.2);
        }}
        
        .footer {{
            text-align: center;
            margin-top: 40px;
            opacity: 0.8;
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🎯 AutoGen故事创作完成</h1>
        </div>
        
        <div class="request-info">
            <strong>📝 创作需求:</strong> {html.escape(user_request)}
        </div>""")
    
    # 显示每个agent的输出
    for entry in result['conversation']:
        agent = entry['agent']
        content = entry['content']
        
        if agent == "Editor":
            icon = "📝"
            section_class = "editor-section"
            title = "编辑 - 故事大纲规划"
        elif agent == "Writer":
            icon = "✍️"
            section_class = "writer-section"
            title = "写作 - 故事创作"
        elif agent == "Reviewer":
            icon = "🔍"
            section_class = "reviewer-section"
            title = "审核 - 最终定稿"
        else:
            icon = "👤"
            section_class = ""
            title = agent
        
        print(f"""
        <div class="agent-section {section_class}">
            <div class="agent-header">
                <span class="agent-icon">{icon}</span>
                {title}
            </div>
            <div class="agent-content">{html.escape(content)}</div>
        </div>""")
    
    # 显示最终故事
    if result.get('final_story'):
        print(f"""
        <div class="final-story">
            <h2>✨ 最终故事作品</h2>
            <div class="final-story-content">{html.escape(result['final_story'])}</div>
        </div>""")
    
    print(f"""
        <div class="footer">
            <a href="?" class="back-btn">🔄 重新创作</a>
            <p>✨ AutoGen三智能体协作完成 | 生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        </div>
    </div>
</body>
</html>""")

def main():
    print_headers()
    
    # 检查请求方法
    request_method = os.environ.get('REQUEST_METHOD', 'GET')
    user_request = ""
    
    if request_method == 'POST':
        # 处理表单提交
        form_data = get_form_data()
        user_request = form_data.get('prompt', '').strip()
    else:
        # 处理GET请求
        query_params = get_query_params()
        user_request = query_params.get('prompt', '').strip()
    
    if user_request:
        # 有用户输入，运行AutoGen系统
        try:
            # 运行AutoGen多智能体系统
            result = asyncio.run(run_autogen_multiagent(user_request))
            show_result_page(user_request, result)
        except Exception as e:
            print(f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AutoGen系统错误</title>
    <style>
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            text-align: center;
            padding: 20px;
        }}
        .error-container {{
            background: rgba(255,255,255,0.1);
            padding: 40px;
            border-radius: 25px;
            border: 1px solid rgba(255,255,255,0.2);
            max-width: 600px;
        }}
        .error-icon {{
            font-size: 4em;
            margin-bottom: 20px;
        }}
        .back-btn {{
            display: inline-block;
            background: linear-gradient(45deg, #4ECDC4, #44A08D);
            color: white;
            padding: 12px 30px;
            text-decoration: none;
            border-radius: 25px;
            font-weight: bold;
            margin-top: 20px;
        }}
    </style>
</head>
<body>
    <div class="error-container">
        <div class="error-icon">⚠️</div>
        <h2>AutoGen系统运行错误</h2>
        <p>错误信息: {html.escape(str(e))}</p>
        <p>请检查系统配置或稍后重试</p>
        <a href="?" class="back-btn">🔄 返回首页</a>
    </div>
</body>
</html>""")
    else:
        # 没有用户输入，显示输入表单
        show_input_form()

if __name__ == "__main__":
    main() 