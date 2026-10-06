#!/usr/bin/env python3
"""Compile every example in RB_Nexus, including micro_ros examples and configured fixtures."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--config-file')
parser.add_argument('--build-root', type=Path)
parser.add_argument('--filter', help='Optional substring filter for sketch names')
parser.add_argument('--cli', default=shutil.which('arduino-cli'))
parser.add_argument('--installed-library', action='store_true', help='Test library inside installed board package')
parser.add_argument('--jobs', type=int, default=4, help='Compiler processes per sketch')
args = parser.parse_args()

cli_bin = args.cli
if not cli_bin:
    parser.error('Arduino CLI not found. Install it or pass --cli /path/to/arduino-cli')
cli = [cli_bin]
if args.config_file:
    cli += ['--config-file', args.config_file]

# Compiler-only fixtures: these GPIOs test custom preprocessor override defines
configured = {
    'Blink': '-DRB_EXAMPLE_LED_PIN=25',
    'DigitalInput': '-DRB_EXAMPLE_INPUT_PIN=27',
    'AnalogInput': '-DRB_EXAMPLE_ADC_PIN=34',
    'PWMOutput': '-DRB_EXAMPLE_PWM_PIN=25',
    'I2CScanner': '-DRB_EXAMPLE_SDA_PIN=21 -DRB_EXAMPLE_SCL_PIN=22',
}

examples = sorted((ROOT / 'libraries/RB_Nexus/examples').rglob('*.ino'))
if args.filter:
    examples = [ex for ex in examples if args.filter.lower() in ex.stem.lower()]
if not examples:
    parser.error('No sketches matched; refusing to report an empty test run as success')

passed = 0
failed = 0
total_runs = 0

print(f"Discovered {len(examples)} example sketch(es) to compile for RB_Nexus (esp32:rb_nexus)...", flush=True)

for example in examples:
    command = cli + ['compile', '--fqbn', 'RB_Nexus:esp32:rb_nexus', '--jobs', str(args.jobs)]
    if not args.installed_library:
        command += ['--library', str(ROOT / 'libraries/RB_Nexus')]
    if args.build_root:
        command += ['--build-path', str(args.build_root / example.stem)]
    else:
        # Arduino invalidates changed objects while reusing the large ESP32 core.
        command += ['--build-path', str(ROOT / '.cache/example-build')]

    print(f"--> Compiling {example.stem}...", end='', flush=True)
    total_runs += 1
    res = subprocess.run(command + [str(example.parent)], capture_output=True, text=True)
    if res.returncode == 0:
        print(" [PASS]")
        passed += 1
    else:
        print(" [FAIL]")
        print(f"Error output for {example.stem}:\n{res.stderr}\n{res.stdout}")
        failed += 1

    if example.stem in configured:
        print(f"--> Compiling {example.stem} (configured GPIO paths)...", end='', flush=True)
        total_runs += 1
        res2 = subprocess.run(command + ['--build-property', 'compiler.cpp.extra_flags=' + configured[example.stem],
                                         str(example.parent)], capture_output=True, text=True)
        if res2.returncode == 0:
            print(" [PASS]")
            passed += 1
        else:
            print(" [FAIL]")
            print(f"Error output for {example.stem} (configured):\n{res2.stderr}\n{res2.stdout}")
            failed += 1

print("\n" + "=" * 50)
print(f"COMPILATION SUMMARY:")
print(f"Total builds: {total_runs}")
print(f"Passed:       {passed}")
print(f"Failed:       {failed}")
print("=" * 50)

if failed > 0:
    sys.exit(1)
sys.exit(0)
