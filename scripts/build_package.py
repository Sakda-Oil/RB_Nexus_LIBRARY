#!/usr/bin/env python3
"""Build a deterministic Arduino board package from the pinned Espressif release."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = (ROOT / 'VERSION').read_text().strip()
UPSTREAM = json.loads((ROOT / 'metadata/upstream.json').read_text())
NOTICE = '# Modified for RB_Nexus on 2026-09-09; based on Espressif Arduino-ESP32 3.3.10.\n'


def build(archive, repository, output):
    if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository):
        raise ValueError('Expected repository in owner/name format')
    payload = archive.read_bytes()
    expected = UPSTREAM['checksum'].split(':', 1)[1].lower()
    if hashlib.sha256(payload).hexdigest() != expected:
        raise ValueError('Upstream archive SHA-256 mismatch')
    if len(payload) != int(UPSTREAM['size']):
        raise ValueError('Upstream archive size mismatch')
    entries = {}
    with zipfile.ZipFile(io.BytesIO(payload)) as z:
        prefix = f"esp32-core-{UPSTREAM['coreVersion']}/"
        for entry in z.infolist():
            if entry.is_dir():
                continue
            if not entry.filename.startswith(prefix):
                raise ValueError('Unexpected archive root')
            relative = entry.filename[len(prefix):]
            if '..' in Path(relative).parts or relative.startswith('/'):
                raise ValueError('Unsafe archive path')
            entries[relative] = (z.read(entry), entry.external_attr)
    original_boards = entries['boards.txt'][0].decode()
    properties = [line for line in original_boards.splitlines() if line.startswith('esp32.')]
    if 'esp32.name=ESP32 Dev Module' not in properties:
        raise ValueError('Upstream ESP32 board definition changed')
    # Preserve every generic ESP32 option, including CPU, flash and partition menus.
    properties = [line.replace('esp32.', 'rb_nexus.', 1) for line in properties]
    replacements = {
        'rb_nexus.name': 'RB_Nexus',
        'rb_nexus.build.board': 'RB_NEXUS',
        'rb_nexus.build.variant': 'rb_nexus',
    }
    properties = [line.split('=', 1)[0] + '=' + replacements[line.split('=', 1)[0]]
                  if line.split('=', 1)[0] in replacements else line for line in properties]
    # A conservative upload speed is the first (default) menu option.
    defaults = ('rb_nexus.menu.UploadSpeed.115200', 'rb_nexus.menu.FlashFreq.40', 'rb_nexus.menu.FlashMode.dio')
    preferred = [line for prefix in defaults for line in properties
                 if line.startswith(prefix + '=') or line.startswith(prefix + '.')]
    properties = preferred + [line for line in properties if line not in preferred]
    menus = {line.split('.')[2] for line in properties if line.startswith('rb_nexus.menu.')}
    menu_headers = [line for line in original_boards.splitlines()
                    if line.startswith('menu.') and line.split('=', 1)[0][5:] in menus]
    entries['boards.txt'] = ((NOTICE + '\n'.join(menu_headers + [''] + properties) + '\n').encode(), 0o100644 << 16)
    platform = entries['platform.txt'][0].decode()
    platform = re.sub(r'^name=.*$', 'name=RB_Nexus ESP32 Boards', platform, count=1, flags=re.M)
    platform = re.sub(r'^version=.*$', f'version={VERSION}', platform, count=1, flags=re.M)
    entries['platform.txt'] = ((NOTICE + platform).encode(), 0o100644 << 16)
    pins = entries['variants/esp32/pins_arduino.h'][0]
    entries['variants/rb_nexus/pins_arduino.h'] = (
        b'// RB_Nexus 0.1: generic ESP32 GPIO aliases only.\n'
        b'// Peripheral connector pin mapping has not been supplied.\n'
        b'// Modified 2026-09-09 from Espressif variants/esp32/pins_arduino.h.\n' + pins, 0o100644 << 16)
    for path in sorted((ROOT / 'libraries').rglob('*')):
        if path.is_file():
            entries[path.relative_to(ROOT).as_posix()] = (path.read_bytes(), 0o100644 << 16)
    for name in ['LICENSE.md', 'NOTICE.md', 'README.md', 'HARDWARE.md']:
        entries[name] = ((ROOT / name).read_bytes(), 0o100644 << 16)
    output.mkdir(parents=True, exist_ok=True)
    archive_name = f'RB_Nexus-esp32-{VERSION}.zip'
    target = output / archive_name
    # ZIP_STORED avoids zlib version differences between macOS and GitHub runners.
    with zipfile.ZipFile(target, 'w', compression=zipfile.ZIP_STORED) as z:
        for name, (data, attr) in sorted(entries.items()):
            item = zipfile.ZipInfo(f'RB_Nexus-esp32-{VERSION}/{name}', (2026, 9, 9, 0, 0, 0))
            item.create_system = 3
            item.external_attr = attr
            z.writestr(item, data)
    repo_url = f'https://github.com/{repository}'
    deps = [dict(d, packager='RB_Nexus') for d in UPSTREAM['toolsDependencies']]
    platform_info = {
        'name': 'RB_Nexus ESP32 Boards', 'architecture': 'esp32', 'version': VERSION,
        'category': 'Contributed',
        'url': f'{repo_url}/releases/download/v{VERSION}/{archive_name}',
        'archiveFileName': archive_name,
        'checksum': 'SHA-256:' + hashlib.sha256(target.read_bytes()).hexdigest(),
        'size': str(target.stat().st_size), 'boards': [{'name': 'RB_Nexus'}],
        'toolsDependencies': deps,
        'help': {'online': repo_url + '/issues'},
    }
    index = {'packages': [{
        'name': 'RB_Nexus', 'maintainer': repository.split('/')[0],
        'websiteURL': repo_url, 'help': {'online': repo_url + '/issues'},
        'platforms': [platform_info], 'tools': UPSTREAM['tools'],
    }]}
    (output / 'package_RB_Nexus_index.json').write_text(json.dumps(index, indent=2) + '\n')
    print(json.dumps({'archive': str(target), 'checksum': platform_info['checksum'],
                      'size': platform_info['size']}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--upstream-archive', type=Path)
    parser.add_argument('--repository', default='Sakda-Oil/RB_Nexus_LIBRARY')
    parser.add_argument('--output', type=Path, default=ROOT / 'dist')
    args = parser.parse_args()
    archive = args.upstream_archive
    if archive is None:
        cache = ROOT / '.cache'
        cache.mkdir(exist_ok=True)
        archive = cache / f"esp32-core-{UPSTREAM['coreVersion']}.zip"
        if not archive.exists():
            urllib.request.urlretrieve(UPSTREAM['url'], archive)
    build(archive, args.repository, args.output)
