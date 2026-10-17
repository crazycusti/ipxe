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
import shutil
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('architecture', choices=['sparc64', 'powerpc'])
    parser.add_argument('kind', choices=['selftests', 'menu', 'network'])
    parser.add_argument('output', type=Path)
    parser.add_argument('--seconds', type=int, default=240)
    args = parser.parse_args()
    if args.kind == 'network' and args.architecture == 'sparc64':
        raise SystemExit('The stock QEMU OpenBIOS SPARC firmware lacks packet I/O; this path is not qualified.')
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
                 ('sparc64-linux-gnu-' if args.architecture == 'sparc64' else 'powerpc-linux-gnu-')]
        target = 'tests.of' if args.kind == 'selftests' else 'ipxe.of' if args.kind == 'network' else 'none.of'
        if args.kind != 'selftests':
            if args.kind == 'network':
                url = 'http://10.0.2.2:' + str(server.server_port)
                script = ('#!ipxe\ndhcp || goto failed\nimgfetch --name payload ' + url +
                          '/payload.bin || goto failed\nsha256sum payload\nimgfree payload\nchain ' + url +
                          '/menu.ipxe || goto failed\necho OFW_NETWORK_OK\nexit\n:failed\necho OFW_NETWORK_FAILED\nexit 1\n')
            else:
                script = menu.decode().replace('echo OFW_HTTP_SCRIPT_OK\n', '')
            embedded = args.output / 'embedded.ipxe'
            embedded.write_text(script)
            build += ['EMBED=' + str(embedded)]
        build += ['bin-' + args.architecture + '-openfirmware/' + target]
        (args.output / 'build-command.json').write_text(json.dumps(build, indent=2) + '\n')
        with (args.output / 'build.log').open('wb') as log:
            subprocess.run(build, check=True, stdout=log, stderr=subprocess.STDOUT)
        binary = args.output / target
        shutil.copy2(ROOT / 'src' / ('bin-' + args.architecture + '-openfirmware') / target, binary)
        if args.architecture == 'sparc64':
            command = ['qemu-system-sparc64', '-M', 'sun4u', '-m', '2048', '-kernel', str(binary), '-nic', 'none']
        else:
            command = ['qemu-system-ppc64', '-M', 'pseries', '-cpu', 'POWER9', '-m', '1024',
                       '-nic', 'user,model=virtio-net-pci,tftp=' + str(args.output) + ',bootfile=' + target, '-boot', 'n']
        command += ['-nographic', '-monitor', 'none', '-no-reboot']
        (args.output / 'command.json').write_text(json.dumps(command, indent=2) + '\n')
        serial = args.output / 'serial.log'
        with serial.open('wb') as log:
            proc = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT)
            end = time.monotonic() + args.seconds
            passed = False
            while time.monotonic() < end and proc.poll() is None:
                time.sleep(.25)
                text = serial.read_text(errors='replace')
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
                  'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(), 'http_requests': requests,
                  'hardware_verified': False, 'operating_system_handoff_verified': False,
                  'firmware_network_boot_verified': args.architecture == 'powerpc' and passed}
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
