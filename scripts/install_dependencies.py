#!/usr/bin/env python3
"""Install pinned real IMU and micro-ROS dependencies for IDE/CI compilation."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
MICROROS_COMMIT = '5917738c089d353c47885a0f0926fa0de1ee6cf2'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cli', default=shutil.which('arduino-cli'))
    parser.add_argument('--config-file')
    args = parser.parse_args()
    if not args.cli:
        parser.error('Install Arduino CLI or pass --cli')
    command = [args.cli]
    if args.config_file:
        command += ['--config-file', args.config_file]
    subprocess.run(command + ['lib', 'install', 'MPU9250@0.4.8', 'Adafruit BNO08x@1.2.7',
                              'Adafruit BusIO@1.17.4', 'Adafruit Unified Sensor@1.1.15'], check=True)
    cache = ROOT / '.cache'
    cache.mkdir(exist_ok=True)
    archive = cache / f'micro_ros_arduino-{MICROROS_COMMIT}.zip'
    if not archive.exists():
        urllib.request.urlretrieve(
            f'https://github.com/micro-ROS/micro_ros_arduino/archive/{MICROROS_COMMIT}.zip', archive)
    env = dict(os.environ, ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL='true')
    subprocess.run(command + ['lib', 'install', '--zip-path', str(archive)], env=env, check=True)

if __name__ == '__main__':
    main()
