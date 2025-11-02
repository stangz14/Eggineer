from googleapiclient.discovery import build
from google.oauth2.service_account import Credentials
from flask import Flask, jsonify, request
from data import Data

class GoogleSheetEditor:

    def __init__(self):
        self._SERVICE_ACCOUNT_FILE = ''
        self._SPREADSHEET_ID = ''
        self._SHEET_NAME = ''
        self.SCOPES = ['https://www.googleapis.com/auth/spreadsheets']
        self.service = self._authenticate_and_build_service()

    def _authenticate_and_build_service(self):
        """Handles authentication and returns the Sheets service object."""
        try:
            credentials = Credentials.from_service_account_file(
                self._SERVICE_ACCOUNT_FILE, scopes=self.SCOPES)
            service = build('sheets', 'v4', credentials=credentials)
            print("Successfully authenticated and built Sheets service.")
            return service
        except Exception as e:
            print(f"Authentication Error: {e}")
            raise

    def _read_data(self, range_name):
        result = self.service.spreadsheets().values().get(
            spreadsheetId=self._SPREADSHEET_ID,
            range=range_name
        ).execute()
        values = result.get('values', [])
        return values

    def create_token(self):
        if not request.is_json:
            return jsonify({"error": "Request must be JSON"}), 400
        req = request.get_json()
        data = Data(type=req["type"], start_time=req["start_time"], end_time=req["end_time"])

        sheet_range = f"{self._SHEET_NAME}!A2"
        body = {
            'values' : [data.display_list()]
        }

        self.service.spreadsheets().values().append(
            spreadsheetId=self._SPREADSHEET_ID,
            range=sheet_range,
            valueInputOption='USER_ENTERED',
            insertDataOption='INSERT_ROWS',
            body=body
        ).execute()

        return jsonify({"token" : data.token})

    def connect_user_token(self, token, user_id):

        sheet_range = f"{self._SHEET_NAME}!A:B"
        values = self._read_data(sheet_range)

        target_row = None
        status_code = 4

        for i, row in enumerate(values):
            if not row:
                continue
            
            token_value = row[0]
            user_value = row[1] if len(row) > 1 else None

            if (token_value == token and not user_value) or (token_value == token and user_value == user_id):
                target_row = i + 1
                status_code = 1
            
            if token_value == token and user_value != user_id and user_value:
                status_code = 2
                break
            
            if user_value == user_id:
                status_code = 3
                break
        if status_code == 1 and target_row:
            update_range = f"{self._SHEET_NAME}!B{target_row}:B{target_row}"
            body = {"values": [[user_id]]}

            self.service.spreadsheets().values().update(
                spreadsheetId=self._SPREADSHEET_ID,
                range=update_range,
                valueInputOption="USER_ENTERED",
                body=body
            ).execute()
            print("success")
        return status_code
    
    def disconnect_user_token(self, token):
        sheet_range = f"{self._SHEET_NAME}!A:B"
        values = self._read_data(sheet_range)

        for i, row in enumerate(values):
            if not row:
                continue

            token_value = row[0]
            if token_value == token:
                update_range = f"{self._SHEET_NAME}!A{i + 1}:B{i + 1}"
                body = {"values": [["", ""]]} 

                self.service.spreadsheets().values().update(
                    spreadsheetId=self._SPREADSHEET_ID,
                    range=update_range,
                    valueInputOption="USER_ENTERED",
                    body=body
                ).execute()
                break

    
    def find_user(self, token):
        sheet_range = f"{self._SHEET_NAME}!A:B"
        values = self._read_data(sheet_range)
        
        for i, row in enumerate(values):
            if not row:
                continue
            
            token_value = row[0]
            user_value = row[1] if len(row) > 1 else None
            if token == token_value:
                return user_value
        else:
            return "NONE"