#!/usr/bin/env python3
"""Build and boot native iPXE; assert real self-test/menu/network results.

Uses only disposable QEMU machines, slirp networking and a temporary HTTP
server bound to localhost. The working LAN and installed bootloaders are unused.
"""
import argparse
import hashlib
import http.server
import json
from pathlib import Path
import re
import resource
import shutil
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('architecture', choices=['sparc32', 'sparc64', 'powerpc', 'hppa'])
    parser.add_argument('kind', choices=['selftests', 'menu', 'input', 'network'])
    parser.add_argument('output', type=Path)
    parser.add_argument('--seconds', type=int, default=600)
    parser.add_argument('--qemu', help='Override the QEMU system emulator executable')
    parser.add_argument('--debug', help='Comma-separated iPXE debug objects')
    parser.add_argument('--trace', action='store_true', help='Capture bounded QEMU exception tracing')
    args = parser.parse_args()
    if args.kind == 'network' and args.architecture in ['sparc32', 'sparc64']:
        raise SystemExit('Network qualification is not available for this target yet.')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    requests = []
    payload = bytes(range(256)) * 64
    digest = hashlib.sha256(payload).hexdigest()
    menu = b'''#!ipxe
echo OFW_HTTP_SCRIPT_OK
menu iPXE Open Firmware test
item test Continue test
choose --default test --timeout 1500 selected || exit 1
iseq ${selected} test || exit 1
echo OFW_MENU_OK
exit
'''

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            data = {'/menu.ipxe': menu, '/payload.bin': payload}.get(self.path)
            requests.append({'path': self.path, 'found': data is not None})
            if data is None:
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def log_message(self, *_args):
            pass

    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    proc = None
    try:
        build = ['make', '-C', str(ROOT / 'src'), '-j8', 'CROSS=' +
                 ({'sparc32': 'sparc64-linux-gnu-', 'sparc64': 'sparc64-linux-gnu-', 'powerpc': 'powerpc-linux-gnu-', 'hppa': 'hppa-linux-gnu-'}[args.architecture])]
        suffix = '.lif' if args.architecture == 'hppa' else '.of'
        platform = 'pdc' if args.architecture == 'hppa' else 'openfirmware'
        target = ('tests' if args.kind == 'selftests' else 'ipxe' if args.kind == 'network' else 'none') + suffix
        if args.debug:
            build += ['DEBUG=' + args.debug]
        if args.kind != 'selftests':
            if args.kind == 'network':
                url = 'http://10.0.2.2:' + str(server.server_port)
                script = ('#!ipxe\ndhcp || goto failed\nimgfetch --name payload ' + url +
                          '/payload.bin || goto failed\nsha256sum payload\nimgfree payload\nchain ' + url +
                          '/menu.ipxe || goto failed\necho OFW_NETWORK_OK\nexit\n:failed\necho OFW_NETWORK_FAILED\nexit 1\n')
            else:
                script = menu.decode().replace('echo OFW_HTTP_SCRIPT_OK\n', '')
                if args.kind == 'input':
                    script = script.replace('item test Continue test', 'item fail Wrong selection\nitem test Continue test')
                    script = script.replace('--default test --timeout 1500', '--default fail --timeout 5000')
            embedded = args.output / 'embedded.ipxe'
            embedded.write_text(script)
            build += ['EMBED=' + str(embedded)]
        build += ['bin-' + args.architecture + '-' + platform + '/' + target]
        (args.output / 'build-command.json').write_text(json.dumps(build, indent=2) + '\n')
        with (args.output / 'build.log').open('wb') as log:
            subprocess.run(build, check=True, stdout=log, stderr=subprocess.STDOUT)
        binary = args.output / target
        shutil.copy2(ROOT / 'src' / ('bin-' + args.architecture + '-' + platform) / target, binary)
        if args.architecture == 'sparc64':
            command = ['qemu-system-sparc64', '-M', 'sun4u', '-m', '2048', '-kernel', str(binary), '-nic', 'none']
        elif args.architecture == 'sparc32':
            command = ['qemu-system-sparc', '-M', 'SS-5', '-m', '128', '-kernel', str(binary), '-nic', 'none']
        elif args.architecture == 'hppa':
            command = ['qemu-system-hppa', '-M', 'B160L', '-m', '128', '-drive', 'if=scsi,format=raw,readonly=on,file=' + str(binary), '-nic', 'none']
            if args.kind == 'network':
                command[-1] = 'user,model=e1000'
        else:
            command = ['qemu-system-ppc64', '-M', 'pseries', '-cpu', 'POWER9', '-m', '1024',
                       '-nic', 'user,model=virtio-net-pci,tftp=' + str(args.output) + ',bootfile=' + target, '-boot', 'n']
        if args.qemu:
            command[0] = args.qemu
        emulator_version = subprocess.check_output([command[0], '--version'], text=True).splitlines()[0]
        command += ['-nographic', '-monitor', 'unix:' + str(args.output / 'monitor.sock') + ',server=on,wait=off', '-no-reboot']
        if args.trace:
            command += ['-d', 'int', '-D', str(args.output / 'exceptions.log')]
        (args.output / 'command.json').write_text(json.dumps(command, indent=2) + '\n')
        serial = args.output / 'serial.log'
        with serial.open('wb') as log:
            def limit_trace():
                resource.setrlimit(resource.RLIMIT_FSIZE, (32 * 1024 * 1024, 32 * 1024 * 1024))
            proc = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=log, stderr=subprocess.STDOUT,
                                    preexec_fn=limit_trace if args.trace else None)
            end = time.monotonic() + args.seconds
            passed = False
            input_sent = False
            while time.monotonic() < end and proc.poll() is None:
                time.sleep(.25)
                text = serial.read_text(errors='replace')
                if args.kind == 'input' and not input_sent and 'Continue test' in text:
                    proc.stdin.write(b'\x1b[B\r')
                    proc.stdin.flush()
                    input_sent = True
                if 'FAILURE:' in text or 'Unhandled Exception' in text or 'OFW_NETWORK_FAILED' in text:
                    break
                if args.kind == 'selftests':
                    passed = bool(re.search(r'OK: all \d+ tests passed', text))
                elif args.kind == 'network':
                    passed = 'OFW_NETWORK_OK' in text and 'OFW_MENU_OK' in text and digest in text
                else:
                    passed = 'OFW_MENU_OK' in text
                if passed:
                    break
        if args.kind == 'network':
            passed = passed and {'/menu.ipxe', '/payload.bin'} <= {r['path'] for r in requests if r['found']}
        result = {'architecture': args.architecture, 'kind': args.kind, 'passed': passed,
                  'emulator_version': emulator_version,
                  'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(), 'http_requests': requests,
                  'hardware_verified': False, 'operating_system_handoff_verified': False,
                  'console_input_verified': args.kind == 'input' and input_sent and passed,
                  'firmware_network_boot_verified': args.architecture == 'powerpc' and passed,
                  'firmware_scsi_palo_boot_verified': args.architecture == 'hppa' and passed}
        (args.output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result), flush=True)
        if not passed:
            raise SystemExit(1)
    finally:
        if proc:
            proc.terminate()
            try:
                proc.wait(5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
