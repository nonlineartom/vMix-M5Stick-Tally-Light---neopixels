#!/usr/bin/env python3
"""
BRONTIDE vMix snapshot proxy.

vMix's HTTP API has no endpoint that returns a JPEG of an input on demand:
`Function=SnapshotInput` writes to disk, and the Web Controller's thumbnail
URLs are GUID-keyed and not stable. This proxy bridges the gap. It runs on
the same machine as vMix, exposes:

    GET /preview/<input>?w=320&h=110

…and on each request asks vMix to snapshot that input, then resizes the
file and returns it as a JPEG. The C6 tally polls this at ~5 fps.

Usage:
    pip install pillow requests
    python proxy.py --vmix http://127.0.0.1:8088 --port 8089

Notes:
- vMix must have the Web Controller enabled (Settings → Web Controller).
- Snapshots are written to vMix's configured snapshot directory; we read
  them back from there. Override --snapshot-dir if it isn't the default.
- A 1-second internal rate limit per input keeps us from hammering disk.
"""

import argparse
import io
import os
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs
from urllib.request import urlopen

from PIL import Image  # pillow

DEFAULT_SNAPSHOT_DIR = os.path.expanduser(
    "~/Documents/vMix Snapshots"
)

_last_snapshot_at: dict[str, float] = {}


def snapshot(vmix_base: str, input_id: str) -> None:
    """Ask vMix to snapshot the given input. Rate-limited to 1 Hz per input."""
    now = time.time()
    if now - _last_snapshot_at.get(input_id, 0) < 1.0:
        return
    _last_snapshot_at[input_id] = now
    url = f"{vmix_base}/api/?Function=SnapshotInput&Input={input_id}"
    try:
        urlopen(url, timeout=0.5).read()
    except Exception as e:
        print(f"snapshot {input_id} failed: {e}")


def latest_snapshot(snapshot_dir: str, input_id: str) -> str | None:
    """Pick the most recently modified .jpg/.png in the snapshot dir."""
    try:
        candidates = [
            os.path.join(snapshot_dir, f)
            for f in os.listdir(snapshot_dir)
            if f.lower().endswith((".jpg", ".jpeg", ".png"))
        ]
    except FileNotFoundError:
        return None
    if not candidates:
        return None
    return max(candidates, key=os.path.getmtime)


def resize_jpeg(path: str, w: int, h: int, quality: int = 70) -> bytes:
    img = Image.open(path).convert("RGB")
    img.thumbnail((w, h), Image.LANCZOS)
    buf = io.BytesIO()
    img.save(buf, format="JPEG", quality=quality, optimize=True)
    return buf.getvalue()


class Handler(BaseHTTPRequestHandler):
    vmix_base: str = ""
    snapshot_dir: str = ""

    def log_message(self, fmt, *args):
        # Quieter logs.
        pass

    def do_GET(self):
        url = urlparse(self.path)
        if not url.path.startswith("/preview/"):
            self.send_error(404)
            return
        input_id = url.path[len("/preview/") :]
        params = parse_qs(url.query)
        w = int(params.get("w", ["320"])[0])
        h = int(params.get("h", ["110"])[0])

        snapshot(self.vmix_base, input_id)
        path = latest_snapshot(self.snapshot_dir, input_id)
        if not path:
            self.send_error(503, "no snapshot yet")
            return
        try:
            jpg = resize_jpeg(path, w, h)
        except Exception as e:
            self.send_error(500, f"decode: {e}")
            return
        self.send_response(200)
        self.send_header("Content-Type", "image/jpeg")
        self.send_header("Content-Length", str(len(jpg)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(jpg)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--vmix", default="http://127.0.0.1:8088",
                    help="vMix Web Controller base URL")
    ap.add_argument("--snapshot-dir", default=DEFAULT_SNAPSHOT_DIR,
                    help="Directory vMix writes snapshots to")
    ap.add_argument("--port", type=int, default=8089)
    ap.add_argument("--bind", default="0.0.0.0")
    args = ap.parse_args()

    Handler.vmix_base = args.vmix.rstrip("/")
    Handler.snapshot_dir = args.snapshot_dir

    print(f"BRONTIDE snapshot proxy on http://{args.bind}:{args.port}")
    print(f"  vMix:        {args.vmix}")
    print(f"  Snapshots:   {args.snapshot_dir}")
    ThreadingHTTPServer((args.bind, args.port), Handler).serve_forever()


if __name__ == "__main__":
    main()
