import SharedArray as sa
import sysv_ipc, json
import os, mmap

parent = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
tickers = os.path.join(parent, "tickers")

try:
    cornev = sysv_ipc.MessageQueue(645, sysv_ipc.IPC_CREX)
except Exception as e:
    cornev = sysv_ipc.MessageQueue(645)

while True:
    message, _ = cornev.receive()
    message = message.decode("utf-8")
    message = json.loads(message)

    token = message.get('token')

    path = f"file://{os.path.join(tickers, str(token))}"
    arr = sa.attach(path)

    arr[0] = message.get('exchange_timestamp')
    arr[1] = message.get('last_traded_price')
    arr[2] = message.get('closed_price')
    arr[3] = message.get('volume_trade_for_the_day')
    arr[4] = message.get('open_interest')
    arr[5] = message.get('total_buy_quantity') 
    arr[6] = message.get('total_sell_quantity') 

    with open(f"./tickers/{token}", "r+b") as f:
        mm = mmap.mmap(f.fileno(), 0)
        mm.flush()
        mm.close()