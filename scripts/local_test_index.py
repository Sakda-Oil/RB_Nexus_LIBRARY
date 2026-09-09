#!/usr/bin/env python3
"""Serve the generated platform locally for a real Boards Manager install test."""
import argparse
import json
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--port', type=int, default=8765)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
index = json.loads((root / 'dist/package_RB_Nexus_index.json').read_text())
for package in index['packages']:
    for platform in package['platforms']:
        platform['url'] = f"http://127.0.0.1:{a.port}/{platform['archiveFileName']}"
(root / 'dist/package_RB_Nexus_test_index.json').write_text(json.dumps(index, indent=2) + '\n')
