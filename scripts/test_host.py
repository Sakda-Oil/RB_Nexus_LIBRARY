#!/usr/bin/env python3
"""Compile the production driver against fake I2C/clock and run regressions."""
import argparse
from pathlib import Path
import shutil
import subprocess
import os

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--compiler', default=shutil.which('g++') or shutil.which('clang++'))
parser.add_argument('--zig', help='Path to zig; use its C++ compiler instead')
args = parser.parse_args()
if not args.compiler and not args.zig:
    parser.error('Install g++/clang++ or specify --zig /path/to/zig')
for name in ['test_driver', 'test_imu', 'test_distance']:
    output = ROOT / '.cache' / (name + ('.exe' if os.name == 'nt' else ''))
    output.parent.mkdir(exist_ok=True)
    cmd = [args.zig, 'c++'] if args.zig else [args.compiler]
    cmd += ['-std=c++17', '-DARDUINO_ARCH_ESP32', '-I'+str(ROOT/'tests/fakes'),
            '-I'+str(ROOT/'libraries/RB_Nexus/src'), str(ROOT/f'tests/{name}.cpp'),
            str(ROOT/'libraries/RB_Nexus/src/RB_Nexus.cpp'), '-o', str(output)]
    if name == 'test_imu':
        cmd += [str(ROOT/'libraries/RB_Nexus/src/RB_Nexus_IMU.cpp')]
    if name == 'test_distance':
        cmd += [str(ROOT/'libraries/RB_Nexus/src/RB_Nexus_Distance.cpp')]
    subprocess.run(cmd, check=True)
    subprocess.run([str(output)], check=True)
