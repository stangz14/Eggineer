from flask import Flask, jsonify, request
from database import GoogleSheetEditor
from line import LineBotApp

class EggineerApi:
    def __init__(self, name):
        self.app = Flask(name)

        self._add_obj()
        self._register_routes()
        
    def _add_obj(self):
        self.database = GoogleSheetEditor()
        self.linebot = LineBotApp(self.database)
    
    def _register_routes(self):
        self.app.add_url_rule("/createtoken", view_func=self.database.create_token, methods=["POST"])
        self.app.add_url_rule("/callback", view_func=self.linebot.callback , methods=["POST"])
        self.app.add_url_rule("/sendsuccess", view_func=self.linebot.send_success , methods=["POST"])
    
    def run(self, host="0.0.0.0", port=5000):
        self.app.run(host=host, port=port)

if __name__ == "__main__":
    app = EggineerApi(__name__)
    app.run()