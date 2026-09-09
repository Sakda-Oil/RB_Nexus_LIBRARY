#!/usr/bin/env python3
"""Compile every example, including configured GPIO paths, without uploading."""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--config-file')
parser.add_argument('--build-root', type=Path)
args = parser.parse_args()
cli = ['arduino-cli']
if args.config_file:
    cli += ['--config-file', args.config_file]
# Compiler-only fixtures: these GPIOs do not assert a RB_Nexus connector mapping.
configured = {
    'Blink': '-DRB_EXAMPLE_LED_PIN=25',
    'DigitalInput': '-DRB_EXAMPLE_INPUT_PIN=27',
    'AnalogInput': '-DRB_EXAMPLE_ADC_PIN=34',
    'PWMOutput': '-DRB_EXAMPLE_PWM_PIN=25',
    'I2CScanner': '-DRB_EXAMPLE_SDA_PIN=21 -DRB_EXAMPLE_SCL_PIN=22',
}
examples = sorted((ROOT / 'libraries/RB_Nexus/examples').glob('*/*.ino'))
for example in examples:
    command = cli + ['compile', '--fqbn', 'RB_Nexus:esp32:rb_nexus']
    if args.build_root:
        command += ['--build-path', str(args.build_root / example.stem)]
    print(f'Compile {example.stem} (default)', flush=True)
    subprocess.run(command + [str(example.parent)], check=True)
    if example.stem in configured:
        print(f'Compile {example.stem} (configured GPIO paths; compile only)', flush=True)
        subprocess.run(command + ['--build-property', 'compiler.cpp.extra_flags=' + configured[example.stem],
                                 str(example.parent)], check=True)
print(f'PASS: {len(examples)} examples, plus {len(configured)} configured GPIO builds', flush=True)
