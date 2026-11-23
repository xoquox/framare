#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Deterministic subprocess fixture. Never uses the network or a real identity."""
import json
import os
from pathlib import Path
import signal
import sys
import time

base = Path(__file__).resolve().parent
args = sys.argv[1:]
(base / 'transport.pid').write_text(str(os.getpid()))
with (base / 'calls.jsonl').open('a') as output:
    output.write(json.dumps(args, ensure_ascii=False) + '\n')

if 'pair' in args:
    name = args[args.index('-frame') + 1]
    path = Path(args[args.index('-config') + 1])
    config = json.loads(path.read_text()) if path.exists() else {'private_key': '1' * 64, 'frames': {}}
    config['frames'][name] = {'peer_id': '2' * 64, 'name': name}
    config.setdefault('default_frame', name)
    path.write_text(json.dumps(config))
    sys.exit(0)

if (base / 'hang').exists():
    signal.signal(signal.SIGINT, lambda *_: sys.exit(1))
    while True:
        time.sleep(0.05)

# Exercise bounded draining of both channels without interpreting their text.
sys.stdout.write('x' * 150000)
sys.stderr.write('y' * 150000)
if (base / 'fail-second').exists():
    count = len((base / 'calls.jsonl').read_text().splitlines())
    sys.exit(1 if count == 2 else 0)
