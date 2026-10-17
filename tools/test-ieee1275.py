#!/usr/bin/env python3
"""Compatibility launcher; native firmware tests now also cover PDC."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name('test-native.py')), run_name='__main__')
