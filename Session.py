from SmartApi import SmartConnect
import SharedArray as sa
import pandas as pd

import os, requests, shutil
import pyotp, json

pd.set_option('display.max_rows', 500)

def ClearFolder(path):
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path, exist_ok=True)

class TickerGenerator:
    def __init__(self, TokenURL):
        self.TokenURL = TokenURL
        self.df = self.PrepareDF()

    def PrepareDF(self):
        try :
            response = requests.get(self.TokenURL)
            JSON = json.loads(response.text)
            df = pd.DataFrame(JSON)

            mask1 = (df['name'].isin(["NIFTY"])) 
            df = df[mask1]

            mask2 = (df["exch_seg"].isin(["NSE", "NFO"])) 
            df = df[mask2]

            mask3 = (df["instrumenttype"].isin(["AMXIDX", "FUTIDX", "OPTIDX"])) 
            df = df[mask3]

            dataType = {'token': int, 'strike': float, 'lotsize': float, 'tick_size': float}
            df = df.apply(lambda col: col.astype(dataType[col.name]) if col.name in dataType else col)

            df["option"] = df["symbol"].apply(lambda x: x[-2:] if isinstance(x, str) else "")
            df["expiry"] = pd.to_datetime(df["expiry"], errors="coerce")
            df["strike"] = pd.to_numeric(df["strike"], errors="coerce").fillna(0).astype(int) // 100

            df3 = df[df["instrumenttype"] == "OPTIDX"]
            df3 = df3[df3["expiry"] == df3["expiry"].min()]

            df2 = df[df["instrumenttype"] == "FUTIDX"]
            df2 = df2[df2["expiry"] == df2["expiry"].min()]

            df1 = df[df["instrumenttype"] == "AMXIDX"].copy()
            df1["expiry"] = df1["expiry"].fillna(pd.to_datetime(df2["expiry"].min()))

            df = pd.DataFrame()
            for i in [df1, df2, df3]:
                i["expiry"] = i["expiry"].dt.date
                df = pd.concat([df, i], ignore_index=True)

            df = df[["strike", "option", "token", "expiry"]]
            df = df.sort_values(by=["strike", "option", "token", "expiry"])
            df.iloc[0, df.columns.get_loc("strike")] = 0.0

            df.to_csv("Ticker.csv", index=False)
            return df

        except requests.exceptions.RequestException as e:
            print(f"Error fetching URL: {e}")

    def PrepareTicker(self):
        ClearFolder("./tickers")
        ClearFolder("./logs")

        tokens = self.df['token'].tolist()
        
        size = 5000
        dtype = 'float64'
        for token in tokens:
            path = f"file://{os.path.join("./tickers", str(token))}"
            try:
                sa.create(path, size, dtype=dtype)
                print(f"Created ticker for token: {token}")
            except FileExistsError:
                print(f"Ticker for token {token} already exists. Skipping.")
            except Exception as e:
                print(f"Error creating ticker for token {token}: {e}")

class SessionGenerator(TickerGenerator):
    def __init__(self, TokenURL, API_KEY):
        super().__init__(TokenURL)

        self.API_KEY = API_KEY
        self.Angel = SmartConnect(API_KEY)
        self.AUTH_TOKEN = self.FEED_TOKEN = None

    def GenerateSession(self, CLIENT_CODE, PASSWORD, TOTP_SECRET):
        super().PrepareTicker()

        session = self.Angel.generateSession(CLIENT_CODE, PASSWORD, pyotp.TOTP(TOTP_SECRET).now())
        self.AUTH_TOKEN = session["data"]["jwtToken"]
        self.FEED_TOKEN = session["data"]["feedToken"]