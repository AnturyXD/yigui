import asyncio

from websockets.server import serve

TCP_HOST = "192.168.4.1"
TCP_PORT = 333
WS_HOST = "127.0.0.1"
WS_PORT = 9000


async def handler(websocket):
    print("WS client connected")
    try:
        reader, writer = await asyncio.open_connection(TCP_HOST, TCP_PORT)
    except Exception as e:
        await websocket.send(f"ERR:TCP_CONNECT_FAIL {e}\r\n")
        await websocket.close()
        return

    async def tcp_to_ws():
        try:
            while True:
                data = await reader.read(1024)
                if not data:
                    break
                await websocket.send(data.decode(errors="ignore"))
        finally:
            await websocket.close()

    async def ws_to_tcp():
        try:
            async for message in websocket:
                writer.write(message.encode())
                await writer.drain()
        finally:
            writer.close()
            await writer.wait_closed()

    await asyncio.gather(tcp_to_ws(), ws_to_tcp())
    print("WS client disconnected")


async def main():
    print(f"Bridge: ws://{WS_HOST}:{WS_PORT} <-> tcp://{TCP_HOST}:{TCP_PORT}")
    async with serve(handler, WS_HOST, WS_PORT):
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())

