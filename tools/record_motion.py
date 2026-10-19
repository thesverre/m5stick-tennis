#!/usr/bin/env python3
"""Record the stick's motion stream while you follow prompts, for tuning the runner's detection.

Put the stick in your pocket as for the runner game, start this, and do what it says when it
says it. The samples and the times of the prompts go to recordings/<name>.json.

    python3 tools/record_motion.py [--host m5stick.local] [--check]
"""
import argparse, base64, json, math, os, shutil, socket, subprocess, sys, time

# (seconds from the start, what to do)
SCRIPT = [
    (0, "Stand still"),
    (4, "Lift the knee on the stick's side, then put it down"),
    (8, "Step LEFT"),
    (11, "Step RIGHT (back to the middle)"),
    (14, "Step RIGHT"),
    (17, "Step LEFT (back to the middle)"),
    (20, "Step LEFT"),
    (23, "Step RIGHT (back to the middle)"),
    (26, "Step RIGHT"),
    (29, "Step LEFT (back to the middle)"),
    (32, "Run on the spot"),
    (40, "Stop. Stand still"),
    (43, "Jump"),
    (46, "Jump"),
    (49, "Squat, and stay down"),
    (52, "Stand up"),
    (55, None),
]


def say(text):
    """Prints a prompt with a beep, and speaks it if the machine can."""
    print(f"\a{text}", flush=True)
    speaker = shutil.which("spd-say") or shutil.which("espeak-ng") or shutil.which("espeak")
    if speaker:
        subprocess.Popen([speaker, text.split("(")[0]], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def connect(host):
    sock = socket.create_connection((host, 80), timeout=5)
    key = base64.b64encode(os.urandom(16)).decode()
    sock.sendall((f"GET /ws HTTP/1.1\r\nHost: {host}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                  f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n").encode())
    head = b""
    while b"\r\n\r\n" not in head:
        head += sock.recv(1024)
    if b" 101 " not in head.split(b"\r\n")[0]:
        sys.exit(f"The stick refused the connection: {head[:80]!r}")
    return sock, head.split(b"\r\n\r\n", 1)[1]


def frames(sock, buf):
    """Yields the text of each message the stick sends (it never masks; long ones come in parts)."""
    def need(n):
        nonlocal buf
        while len(buf) < n:
            chunk = sock.recv(65536)
            if not chunk:
                raise EOFError
            buf += chunk

    message = b""
    try:
        while True:
            need(2)
            fin, op, n, at = buf[0] & 0x80, buf[0] & 0x0F, buf[1] & 0x7F, 2
            if n == 126:
                need(4)
                n, at = int.from_bytes(buf[2:4], "big"), 4
            elif n == 127:
                need(10)
                n, at = int.from_bytes(buf[2:10], "big"), 10
            need(at + n)
            payload, buf = buf[at:at + n], buf[at + n:]
            if op in (0, 1):
                message += payload
                if fin:
                    yield message.decode()
                    message = b""
    except EOFError:
        return


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="m5stick.local")
    ap.add_argument("--check", action="store_true", help="just check the link for 2 seconds")
    args = ap.parse_args()
    script = [(0, "Checking the link"), (2, None)] if args.check else SCRIPT

    sock, rest = connect(args.host)
    samples, prompts, seen = [], [], -1
    start, step, still = None, 0, 0
    if not args.check:
        say("Connected. Stand still, facing the screen: it starts once you've been still for 2 seconds.")
    for text in frames(sock, rest):
        now = time.monotonic()
        rows = json.loads(text)["s"]
        if start is None:
            if len(rows) > 8:
                continue  # the history the stick sends first
            # Wait for 2 seconds without movement, so the recording starts from a clean stand
            for row in rows:
                quiet = abs(math.hypot(*row[5:8]) - 1) < 0.06 and math.hypot(*row[2:5]) < 25
                still = still + 1 if quiet else 0
            if not args.check and still < 200:
                continue
            start = now
        t = now - start
        for row in rows:
            if row[0] > seen:
                seen = row[0]
                samples.append(row)
        while step < len(script) and t >= script[step][0]:
            what = script[step][1]
            if what is None:
                break
            prompts.append({"at": script[step][0], "device_t": rows[-1][1], "what": what})
            say(f"[{script[step][0]:2d} s] {what}" if args.check else what)
            step += 1
        if step < len(script) and script[step][1] is None and t >= script[step][0]:
            break
    if args.check:
        print(f"OK: {len(samples)} samples in 2 s")
        return
    os.makedirs("recordings", exist_ok=True)
    path = time.strftime("recordings/motion-%Y%m%d-%H%M%S.json")
    with open(path, "w") as f:
        json.dump({"columns": ["seq", "t", "gx", "gy", "gz", "ax", "ay", "az", "btn"], "prompts": prompts, "samples": samples}, f)
    print(f"Done. Saved {len(samples)} samples to {path}")


if __name__ == "__main__":
    main()
