#!/usr/bin/env python3
"""Wrapper executing compile_examples.py for all examples."""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
res = subprocess.run([sys.executable, str(ROOT / "compile_examples.py")] + sys.argv[1:])
sys.exit(res.returncode)
