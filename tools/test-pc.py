#!/usr/bin/env python3
"""Regress PC firmware PXE and menus without changing installed loaders."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('firmware', choices=['bios', 'uefi'])
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    script = out / 'menu.ipxe'
    script.write_text('''#!ipxe
menu iPXE PC regression
item test Continue test
choose --default test --timeout 1500 selected || exit 1
iseq ${selected} test || exit 1
echo PC_MENU_OK
exit
''')
    target = ('bin-i386-pcbios/ipxe.pxe' if args.firmware == 'bios'
              else 'bin-x86_64-efi/ipxe.efi')
    build = ['make', '-C', str(ROOT / 'src'), '-j8',
             'EXTRA_CFLAGS=-DCONSOLE_SERIAL', 'EMBED=' + str(script), target]
    (out / 'build-command.json').write_text(json.dumps(build, indent=2) + '\n')
    with (out / 'build.log').open('wb') as log:
        subprocess.run(build, check=True, stdout=log, stderr=subprocess.STDOUT)
    binary = out / Path(target).name
    shutil.copy2(ROOT / 'src' / target, binary)
    command = ['qemu-system-x86_64', '-machine', 'q35,accel=kvm,smm=off',
               '-cpu', 'host', '-m', '512', '-boot', 'n', '-nic',
               'user,model=e1000,tftp=' + str(out) + ',bootfile=' + binary.name,
               '-display', 'none', '-serial', 'file:' + str(out / 'serial.log'),
               '-monitor', 'none', '-no-reboot']
    if args.firmware == 'uefi':
        shutil.copy2('/usr/share/OVMF/OVMF_VARS_4M.fd', out / 'vars.fd')
        command += ['-drive', 'if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd',
                    '-drive', 'if=pflash,format=raw,file=' + str(out / 'vars.fd')]
    (out / 'command.json').write_text(json.dumps(command, indent=2) + '\n')
    passed = False
    with (out / 'qemu.log').open('wb') as log:
        proc = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT)
        try:
            end = time.monotonic() + 120
            while proc.poll() is None and time.monotonic() < end:
                time.sleep(.25)
                serial = out / 'serial.log'
                if serial.exists() and 'PC_MENU_OK' in serial.read_text(errors='replace'):
                    passed = True
                    break
        finally:
            proc.terminate()
            try:
                proc.wait(5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
    result = {'firmware': args.firmware, 'passed': passed,
              'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
              'hardware_verified': False}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    if not passed:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
