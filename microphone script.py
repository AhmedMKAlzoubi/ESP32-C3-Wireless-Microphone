import socket
import sounddevice as sd
import numpy as np

IP    = "0.0.0.0"   # listen on all interfaces
PORT  = 5005        # must match udpPort
FS    = 16000       # must match SAMPLE_RATE
BLOCK = 160         # samples per packet

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((IP, PORT))
sock.setblocking(False)

print(f"Listening on UDP port {PORT} @ {FS} Hz")

def callback(outdata, frames, time, status):
    try:
        data, addr = sock.recvfrom(BLOCK * 2)
    except BlockingIOError:
        outdata[:] = 0
        return

    if len(data) < BLOCK * 2:
        buf = bytearray(BLOCK * 2)
        buf[:len(data)] = data
        data = bytes(buf)

    samples = np.frombuffer(data, dtype=np.int16)
    outdata[:] = samples.reshape(-1, 1)

with sd.OutputStream(channels=1,
                     samplerate=FS,
                     dtype='int16',
                     blocksize=BLOCK,
                     callback=callback):
    print("Streaming... Ctrl+C to stop")
    while True:
        pass