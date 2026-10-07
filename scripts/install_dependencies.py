#!/usr/bin/env python3
"""Install pinned real IMU and micro-ROS dependencies for IDE/CI compilation."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import urllib.request
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
MICROROS_COMMIT = '5917738c089d353c47885a0f0926fa0de1ee6cf2'
LIBRARIES = json.loads((ROOT / 'metadata/library_dependencies.json').read_text())
HEADERS = {'MPU9250': 'MPU9250.h', 'Adafruit BNO08x': 'Adafruit_BNO08x.h',
           'Adafruit BusIO': 'Adafruit_I2CDevice.h', 'Adafruit Unified Sensor': 'Adafruit_Sensor.h',
           'micro_ros_arduino': 'micro_ros_arduino.h'}


def version_tuple(value):
    match = re.fullmatch(r'(\d+)\.(\d+)\.(\d+)', value)
    return tuple(map(int, match.groups())) if match else None


def dependency_status(inventory, include_microros=True):
    """Check versions and actual headers; report missing/broken installs explicitly."""
    required = LIBRARIES + ([{'name': 'micro_ros_arduino', 'version': '2.0.8-jazzy'}]
                            if include_microros else [])
    results = []
    for requirement in required:
        name, required_version = requirement['name'], requirement['version']
        candidates = [entry['library'] for entry in inventory.get('installed_libraries', [])
                      if entry.get('library', {}).get('name') == name]
        acceptable = []
        for library in candidates:
            version = library.get('version', '')
            source = library.get('source_dir')
            if not source or not (Path(source) / HEADERS[name]).is_file():
                continue
            if name == 'micro_ros_arduino':
                matches = version == required_version
            else:
                found = version_tuple(version)
                matches = found is not None and found >= version_tuple(required_version)
            if matches:
                acceptable.append(library)
        if acceptable:
            versions = ', '.join(lib['version'] for lib in acceptable)
            detail = f'found {versions}; tested {required_version}'
            if name == 'micro_ros_arduino':
                detail += ' (version/header check only; installer pins the Jazzy commit)'
            results.append((True, name, detail))
        else:
            found = ', '.join(lib.get('version', '?') for lib in candidates) or 'not installed'
            results.append((False, name, f'install {required_version}; found {found} or missing header'))
    return results


def check_dependencies(command, include_microros=True):
    result = subprocess.run(command + ['lib', 'list', '--json'], check=True,
                            capture_output=True, text=True, encoding='utf-8')
    inventory = json.loads(result.stdout)
    if not isinstance(inventory, dict):
        raise ValueError('Unexpected Arduino CLI library inventory; cannot verify dependencies')
    statuses = dependency_status(inventory, include_microros)
    for ok, name, detail in statuses:
        print(f'{"OK" if ok else "MISSING/INCOMPATIBLE"}: {name}: {detail}', flush=True)
    return all(ok for ok, _, _ in statuses)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cli', default=shutil.which('arduino-cli'))
    parser.add_argument('--config-file')
    parser.add_argument('--check-only', action='store_true', help='Read-only check; exit 1 if a dependency is missing or incompatible')
    parser.add_argument('--skip-microros', action='store_true', help='Check/install only IMU dependencies; micro-ROS is needed only for ROS sketches')
    args = parser.parse_args()
    if not args.cli:
        parser.error('Install Arduino CLI or pass --cli')
    command = [args.cli]
    if args.config_file:
        command += ['--config-file', args.config_file]
    complete = check_dependencies(command, not args.skip_microros)
    if args.check_only:
        if not complete:
            print('Install the listed libraries in Arduino Library Manager (select Install All), or rerun this command without --check-only.', flush=True)
            print('For ROS sketches, use this installer for the tested micro_ros_arduino Jazzy snapshot.', flush=True)
        return 0 if complete else 1
    subprocess.run(command + ['lib', 'install'] +
                   [f"{lib['name']}@{lib['version']}" for lib in LIBRARIES], check=True)
    if args.skip_microros:
        return 0 if check_dependencies(command, False) else 1
    cache = ROOT / '.cache'
    cache.mkdir(exist_ok=True)
    archive = cache / f'micro_ros_arduino-{MICROROS_COMMIT}.zip'
    if not archive.exists():
        urllib.request.urlretrieve(
            f'https://github.com/micro-ROS/micro_ros_arduino/archive/{MICROROS_COMMIT}.zip', archive)
    env = dict(os.environ, ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL='true')
    subprocess.run(command + ['lib', 'install', '--zip-path', str(archive)], env=env, check=True)
    return 0 if check_dependencies(command) else 1

if __name__ == '__main__':
    sys.exit(main())
