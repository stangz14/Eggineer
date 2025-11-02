from flask import Flask, request, abort, jsonify
from linebot import LineBotApi, WebhookHandler
from linebot.exceptions import InvalidSignatureError
from linebot.models import MessageEvent, TextMessage, TextSendMessage

class LineBotApp:
    def __init__(self, database):
        self._CHANNEL_ACCESS_TOKEN = ''
        self._CHANNEL_SECRET = ''

        self._line_bot_api = LineBotApi(self._CHANNEL_ACCESS_TOKEN)
        self._handler = WebhookHandler(self._CHANNEL_SECRET)

        self._database = database
        self._setup_handlers()


    def _setup_handlers(self):
        """Registers the message event handler with the Line handler."""
        self._handler.add(MessageEvent, message=TextMessage)(self.handle_message)

    def callback(self):
        signature = request.headers.get('X-Line-Signature')
        body = request.get_data(as_text=True)

        try:
            self._handler.handle(body, signature)
        except InvalidSignatureError:
            print("Invalid signature. Check your Channel Secret.")
            abort(400)
        return 'OK'

    def handle_message(self, event, *args, **kwargs):
        if isinstance(event.message, TextMessage):
            user_text = event.message.text.split(":")
            if user_text[0] == "Login":
                reply_text = self._user_connecting( user_text[1], event.source.user_id)
            else:
                reply_text = "คำสั่งผิดพลาดโปรดลองใหม่ภายหลัง"

        self._line_bot_api.reply_message(
            event.reply_token,
            TextSendMessage(text=reply_text)
        )

    def _user_connecting(self, token, user_id):
        status = self._database.connect_user_token(token, user_id);
        reply_text = "เกิดข้อผิดพลาดทำให้ไม่สามารถเข้าสู่ระบบได้"
        match status:
            case 1:
                reply_text = "เข้าสู่ระบบสำเร็จ"
            case 2:
                reply_text = "Token นี้ได้รับการเชื่อมต่อแล้ว"
            case 3:
                reply_text = "ผู้ใช้งานนี้ได้รับการเชื่อมต่อแล้ว"
            case 4:
                reply_text = "เกิดข้อผิดพลาด ไม่สามารถเข้าสู่ระบบได้"
        return reply_text
    
    def send_success(self):
        if not request.is_json:
            return jsonify({"error": "Request must be JSON"}), 400
        req = request.get_json()
        user_id = self._database.find_user(req["token"])
        self._database.disconnect_user_token(req["token"])
        self._line_bot_api.push_message(user_id, TextSendMessage(text="ไข่ของคุณพร้อมรับประทานแล้ว"))
        return jsonify({"status" : "success"})