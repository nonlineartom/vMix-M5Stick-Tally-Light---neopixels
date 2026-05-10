# vmix-snapshot-proxy

Tiny HTTP shim that lets the BRONTIDE C6 tally pull a low-fps preview JPEG
of any vMix input. Runs on the vMix host.

## Why this exists

vMix has no HTTP endpoint that returns a JPEG for an arbitrary input on
demand. `Function=SnapshotInput` writes to disk; the Web Controller's
thumbnails are GUID-keyed and not a stable API. This script bridges that
gap with ~50 lines of Python.

## Setup

```sh
pip install pillow requests
python proxy.py --vmix http://127.0.0.1:8088 --port 8089
```

Then in the C6 tally web UI set the preview URL to:

    http://<vmix-host-ip>:8089/preview/1?w=320&h=110

…replacing `1` with the input number you want to monitor (or use `0` for
program-out, depending on your vMix configuration).

The default snapshot directory is `~/Documents/vMix Snapshots`. Override
with `--snapshot-dir` if vMix is configured elsewhere.

## Notes

- Snapshot requests are rate-limited to 1 Hz per input, so polling at
  5–15 Hz from the tally is safe; you'll just see the same frame
  reflected to you in between.
- Pillow's resize is fast enough on a normal CPU to keep up with
  multiple tallies in parallel.
- Consider running this as a Windows service (NSSM) so it survives
  reboots; one process serves all the tallies on the network.

## Future

If a future vMix release exposes a direct snapshot endpoint, delete this
proxy and point the tally directly at vMix.
