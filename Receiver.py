import SharedArray as sa
import sysv_ipc, json
import os, mmap

try:
    cornev = sysv_ipc.MessageQueue(645, sysv_ipc.IPC_CREX)
except Exception as e:
    cornev = sysv_ipc.MessageQueue(645)

while True:
    message, _ = cornev.receive()
    message = message.decode("utf-8")
    message = json.loads(message)

    token = message.get('token')

    path = f"file://{os.path.join('./tickers', str(token))}"
    arr = sa.attach(path)

    arr[0] = message.get('exchange_timestamp')
    arr[1] = message.get('last_traded_price')

    with open(f"./tickers/{token}", "r+b") as f:
        mm = mmap.mmap(f.fileno(), 0)
        mm.flush()
        mm.close()