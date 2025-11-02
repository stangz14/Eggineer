from secrets import token_hex;
import datetime

class Data:
    def __init__(self,type="Boild", start_time="00:00:00", end_time="00:00:00"):
        self.token = token_hex(16);
        self.uid = "";
        self.date = datetime.datetime.now().strftime("%d/%m/%Y");
        self.type = type
        self.start_time = start_time;
        self.end_time = end_time;
    
    def print_data(self):
        print(f"Token : {self.token}")
        print(f"UID : {self.uid}")
        print(f"Date : {self.date}")
        print(f"Time Start : {self.start_time}")
        print(f"Time End : {self.end_time}")
    
    def display_list(self) -> list:
        return [self.token, self.uid, self.type, self.date, self.start_time, self.end_time]