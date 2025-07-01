from SmartApi.smartWebSocketV2 import SmartWebSocketV2
from dotenv import load_dotenv
from logzero import logger

import sysv_ipc, json, threading, os

from Session import SessionGenerator

correlation_id = "abc123"
mode = 3

load_dotenv()

class DataFeeder(SessionGenerator):
    def __init__(self, TokenURL, API_KEY,  CLIENT_CODE):
        super().__init__(TokenURL, API_KEY)
        self.CLIENT_CODE = CLIENT_CODE

    def PrepareQueueList(self, PASSWORD, TOTP_SECRET):
        super().GenerateSession(self.CLIENT_CODE, PASSWORD, TOTP_SECRET)
        Token = list(map(str, self.df["token"].tolist()))

        token_list1 = [
            {
                "exchangeType": 1, 
                "tokens": [Token[0]]
            }
        ]

        token_list2 = [
            {
                "exchangeType": 2, 
                "tokens": Token[1 : ]
            }
        ]

        try:
            queue = sysv_ipc.MessageQueue(645, sysv_ipc.IPC_CREX)
        except Exception as e:
            queue = sysv_ipc.MessageQueue(645)

        return token_list1, token_list2, queue
    
    def SendData(self, PASSWORD, TOTP_SECRET):
        l1, l2, Queue = self.PrepareQueueList(PASSWORD, TOTP_SECRET)

        sws = SmartWebSocketV2(self.AUTH_TOKEN, self.API_KEY, self.CLIENT_CODE, self.FEED_TOKEN)

        def on_data(wsapp, message):
            data = json.dumps(message)
            Queue.send(data)
            print(data)

        def on_open(wsapp):
            logger.info("WebSocket Open")
            sws.subscribe(correlation_id, mode, l1)
            sws.subscribe(correlation_id, mode, l2)

        def on_error(wsapp, error):
            logger.error(f"WebSocket Error: {repr(error)}")

        def on_close(wsapp):
            logger.warning("WebSocket Closed")

        sws.on_open = on_open
        sws.on_data = on_data
        sws.on_error = on_error
        sws.on_close = on_close

        logger.info("Connecting to WebSocket...")
        threading.Thread(target=sws.connect).start()

TokenURL = "https://margincalculator.angelbroking.com/OpenAPI_File/files/OpenAPIScripMaster.json"

API_KEY = os.getenv("API_KEY")
PASSWORD = os.getenv("PASSWORD")
TOTP_SECRET = os.getenv("TOKEN")
CLIENT_CODE = os.getenv("CLIENT_CODE")

if __name__ == "__main__":
    DF = DataFeeder(TokenURL, API_KEY, CLIENT_CODE)
    DF.SendData(PASSWORD, TOTP_SECRET)