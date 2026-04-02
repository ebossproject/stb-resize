#!/usr/bin/env python3
# Copyright (c) 2026 Cromulence
import socket
import io
from PIL import Image
import yaml
import sys
import time
import statistics
import os

RUNS = 20

SNAPSHOT = (os.getenv("SNAPSHOT", "false").upper() == "TRUE")
HOST = os.getenv("HOST", "localhost")
PORT = int(os.getenv("PORT", 8080))

def get_png_bytes():
    img = Image.new("RGB", (20, 20), (255, 255, 255))
    buf = io.BytesIO()
    img.save(buf, format="PNG")
    return buf.getvalue()

def get_yaml():
    doc = {
        "output": "jpg",
        "commands": [
            {
                "resize": {
                    "channels": 3,
                    "edge": "wrap",
                    "height": 10,
                    "width": 10
                }
            }
        ]
    }
    return yaml.dump(doc, default_flow_style=False)

class ServerClosedEarly(Exception):
    pass

def send_to_server(yaml_str, img_bytes):
    yaml_bytes = yaml_str.encode("utf-8")
    msg = (len(yaml_bytes).to_bytes(4, "big") +
           len(img_bytes).to_bytes(4, "big") +
           yaml_bytes + img_bytes)

    with socket.create_connection((HOST, PORT), timeout=5) as s:
        s.sendall(msg)
        outimg = b""
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            outimg += chunk
    if not outimg:
        raise ServerClosedEarly("Server closed connection without sending any image data (likely crash or error)")
    return outimg

def poll():
    yaml_str = get_yaml()
    img_bytes = get_png_bytes()

    wall_start = time.perf_counter()
    cpu_start = time.process_time()

    try:
        outimg = send_to_server(yaml_str, img_bytes)
    except ServerClosedEarly as e:
        print(f"FAIL: {e}", file=sys.stderr)
        return None
    except Exception as e:
        print("ERROR:", e, file=sys.stderr)
        return None

    wall_elapsed = time.perf_counter() - wall_start
    cpu_elapsed = time.process_time() - cpu_start

    print(f"Received image of ({len(outimg)} bytes) in {wall_elapsed:.6f}s wall, {cpu_elapsed:.6f}s cpu")
    return wall_elapsed, cpu_elapsed

def calc_stats(times):
    if not times:
        return 0, 0, 0, 0
    return (
        min(times),
        max(times),
        statistics.mean(times),
        statistics.stdev(times) if len(times) > 1 else 0
    )

def connect_socket(sock: socket.socket) -> bool:
    status = False

    try:
        sock.connect((HOST, 9040))
    except socket.timeout:
        print("Snapshot Connection Failure: connection failed")
    except Exception as e:
        print(f"Snapshot Connection Failure: {str(e)}")
    else:
        status = True

    return status

def manage_socket(sock: socket.socket) -> bool:
    sockfile = sock.makefile('r')
    done = False
    fail = False

    try:
        for line in sockfile:
            if (done := "snapshot-complete" in line):
                print("Flag 'snapshot-complete' received from socket")
                break
            print(line.rstrip())
    except Exception as e:
        print(f"Snapshot Socket Failure: {str(e)}")
        fail = True

    if not done or fail:
        return False

    return True

def trigger_snapshot() -> bool:
    MINUTE = 60

    s = socket.socket()
    s.settimeout(MINUTE * 10)

    if not connect_socket(s):
        return False

    if not manage_socket(s):
        return False

    return True

def main():
    wall_times = []
    cpu_times = []

    for i in range(RUNS):
        print(f"Run {i+1}:", end=" ")
        result = poll()
        if result:
            wall, cpu = result
            wall_times.append(wall)
            cpu_times.append(cpu)
            print(f"wall={wall:.6f} s, cpu={cpu:.6f} s")
        else:
            print("FAILED")

    if not wall_times:
        print("No successful runs.")
        sys.exit(1)

    wmin, wmax, wavg, wstd = calc_stats(wall_times)
    cmin, cmax, cavg, cstd = calc_stats(cpu_times)

    print(f"\nWall: min={wmin:.6f} s, max={wmax:.6f} s, avg={wavg:.6f} s, stddev={wstd:.6f} s")
    print(f"CPU:  min={cmin:.6f} s, max={cmax:.6f} s, avg={cavg:.6f} s, stddev={cstd:.6f} s")

    snapshotSuccess = trigger_snapshot() if SNAPSHOT else False

    if SNAPSHOT and not snapshotSuccess:
        print("\nSnapshot trigger failure")
        sys.exit(1)
    elif SNAPSHOT:
        print("\nSnapshot trigger accepted")

    sys.exit(0)

if __name__ == "__main__":
    main()
