#!/usr/bin/env python3
"""Check local documentation links, UTF-8 and complete README sketch parity."""
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlsplit, unquote
import re

ROOT = Path(__file__).resolve().parents[1]
class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.targets = []
    def handle_starttag(self, tag, attrs):
        for name, value in attrs:
            if name in ('href', 'src') and value:
                self.targets.append(value)

count = 0
for path in (ROOT / 'docs').rglob('*.html'):
    parser = Links()
    parser.feed(path.read_text(encoding='utf-8'))
    for target in parser.targets:
        url = urlsplit(target)
        if url.scheme or url.netloc or not url.path:
            continue
        resolved = (path.parent / unquote(url.path)).resolve()
        assert resolved.exists(), f'{path}: broken link {target}'
        count += 1
readme = (ROOT / 'README.md').read_text(encoding='utf-8')
sketch = re.findall(r'```cpp\n(.*?)```', readme, re.S)[0]
assert sketch.strip() == (ROOT/'libraries/RB_Nexus/examples/IMURaw/IMURaw.ino').read_text(encoding='utf-8').strip()
assert 'support.dummy' not in '\n'.join(p.read_text(encoding='utf-8') for p in (ROOT/'libraries').rglob('*.ino'))
print(f'PASS: {count} local HTML links, UTF-8 and README sketch parity')
