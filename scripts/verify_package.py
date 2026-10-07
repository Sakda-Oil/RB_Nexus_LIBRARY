#!/usr/bin/env python3
"""Check archive integrity and resolve every platform tool dependency."""
import hashlib
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
index = json.loads((ROOT / 'dist/package_RB_Nexus_index.json').read_text())
pkg = index['packages'][0]
platform = pkg['platforms'][0]
archive = ROOT / 'dist' / platform['archiveFileName']
assert str(archive.stat().st_size) == platform['size']
assert 'SHA-256:' + hashlib.sha256(archive.read_bytes()).hexdigest() == platform['checksum']
assert platform['architecture'] == 'esp32'
assert platform['name'] == 'RB_Nexus'
assert platform['boards'] == [{'name': 'RB_Nexus'}]
assert platform['libraryDependencies'] == json.loads((ROOT / 'metadata/library_dependencies.json').read_text())
lookup = {(t['name'], t['version']): t for t in pkg['tools']}
for dep in platform['toolsDependencies']:
    assert dep['packager'] == pkg['name']
    tool = lookup[(dep['name'], dep['version'])]
    for system in tool['systems']:
        assert system['url'].startswith('https://')
        assert system['checksum'].startswith('SHA-256:')
        assert int(system['size']) > 0
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    root = f"RB_Nexus-esp32-{platform['version']}/"
    for file in ['boards.txt', 'platform.txt', 'cores/esp32/Arduino.h',
                 'variants/rb_nexus/pins_arduino.h', 'LICENSE.md',
                 'libraries/RB_Nexus/src/RB_Nexus.h']:
        assert root + file in z.namelist(), file
    assert 'name=RB_Nexus\n' in z.read(root + 'platform.txt').decode()
    for example in (ROOT / 'libraries/RB_Nexus/examples').rglob('*.ino'):
        assert root + example.relative_to(ROOT).as_posix() in z.namelist()
    for source in (ROOT / 'libraries/RB_Nexus/src').glob('*'):
        assert z.read(root + source.relative_to(ROOT).as_posix()) == source.read_bytes(), source
    boards = z.read(root + 'boards.txt').decode()
    props = dict(line.split('=', 1) for line in boards.splitlines() if '=' in line and not line.startswith('#'))
    assert props['rb_nexus.name'] == 'RB_Nexus'
    assert props['rb_nexus.build.mcu'] == 'esp32'
    assert props['rb_nexus.build.variant'] == 'rb_nexus'
    speed_keys = [k for k in props if k.startswith('rb_nexus.menu.UploadSpeed.')]
    assert speed_keys[0] == 'rb_nexus.menu.UploadSpeed.115200'
    assert next(k for k in props if k.startswith('rb_nexus.menu.FlashFreq.')) == 'rb_nexus.menu.FlashFreq.40'
    assert next(k for k in props if k.startswith('rb_nexus.menu.FlashMode.')) == 'rb_nexus.menu.FlashMode.dio'
print('PASS: archive checksum, layout, board defaults and all tool dependencies')
