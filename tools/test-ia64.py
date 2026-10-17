#!/usr/bin/env python3
"""Qualify IA-64 EFI iPXE in a disposable, isolated QEMU machine.

The experimental emulator has no firmware network bootstrap. iPXE starts
from a fresh FAT disk with a temporary write overlay. Network tests then use real emulated NIC
traffic through slirp and verify a downloaded payload inside iPXE.
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
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('kind', choices=['menu', 'input', 'selftests', 'network', 'snp-network', 'deployed-menu', 'linux'])
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--qemu', required=True)
    p.add_argument('--firmware', type=Path, required=True)
    p.add_argument('--cross', required=True)
    p.add_argument('--machine', default='itanium2-vpc')
    p.add_argument('--seconds', type=int, default=600)
    p.add_argument('--debug')
    p.add_argument('--chain-efi', type=Path)
    p.add_argument('--iso', type=Path, help='Optional read-only OS live root')
    p.add_argument('--kernel',type=Path)
    p.add_argument('--initrd',type=Path)
    p.add_argument('--rootfs',type=Path)
    p.add_argument('--grub-dir',type=Path)
    p.add_argument('--memory',default='512M')
    p.add_argument('--expected-rootfs',type=Path,help='Published EFI already contains the HTTP root prefix')
    p.add_argument('--trace', action='store_true')
    args = p.parse_args()
    if args.kind in ('deployed-menu','linux') and not (args.chain_efi or args.rootfs):
        p.error(args.kind+' requires --chain-efi')
    if args.rootfs and (args.kind!='linux' or not all([args.kernel,args.initrd,args.grub_dir])):
        p.error('--rootfs needs linux, --kernel, --initrd and --grub-dir')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    requests = []
    network = args.kind in ('network', 'snp-network', 'deployed-menu','linux')
    extra_files = {}
    payload = bytes((i * 29 + 17) & 255 for i in range(16384))
    digest = hashlib.sha256(payload).hexdigest()
    marker = 'IA64_TEST_OK'
    menu = ('#!ipxe\nmenu IA-64 EFI iPXE\nitem test Continue test\n'
            'choose --default test --timeout 1500 selected || goto failed\n'
            'iseq ${selected} test || goto failed\necho ' + marker + '\nexit\n'
            ':failed\necho IA64_TEST_FAILED\nexit 1\n')

    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            requests.append(self.path)
            data = {'/payload.bin': payload, '/menu.ipxe': menu.encode(), **extra_files}.get(self.path)
            if data is None:
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header('Content-Length', str(data.stat().st_size if isinstance(data,Path) else len(data)))
            self.end_headers()
            if isinstance(data,Path):
                with data.open('rb') as source:shutil.copyfileobj(source,self.wfile,1024*1024)
            else:self.wfile.write(data)

        def log_message(self, *_args):
            pass

    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    proc = None
    try:
        if args.rootfs:
            url='http://10.0.2.2:'+str(server.server_port)
            rd=args.output/'network-initrd'
            metadata=subprocess.check_output(['python3',str(ROOT/'tools/wrap-netboot-initrd.py'),str(args.initrd),str(rd),'--root-url',url+'/root.squashfs','--sha256',hashlib.sha256(args.rootfs.read_bytes()).hexdigest(),'--report-url',url+'/root-verified'],text=True)
            (args.output/'initrd-provenance.json').write_text(metadata)
            extra_files.update({'/root.squashfs':args.rootfs,'/root-verified':b''})
            cfg=args.output/'grub.cfg'
            cfg.write_text('echo IA64_GRUB_ENTERED\nlinux (memdisk)/gentoo root=/dev/ram0 init=/linuxrc rdinit=/netboot-init looptype=squashfs loop=/netboot-root/image.squashfs.img cdroot console=ttyS0,115200n8\ninitrd (memdisk)/gentoo.igz\necho IA64_GRUB_KERNEL_READY\nboot\n')
            args.chain_efi=args.output/'linux-http.efi'
            subprocess.run(['grub-mkstandalone','-O','ia64-efi','-d',str(args.grub_dir),'-o',str(args.chain_efi),'--modules=linux memdisk tar gzio','boot/grub/grub.cfg='+str(cfg),'gentoo='+str(args.kernel),'gentoo.igz='+str(rd)],check=True)
        build = ['make', '-C', str(ROOT / 'src'), '-j8', 'CROSS=' + args.cross]
        target = ('tests' if args.kind == 'selftests' else
                  'intel' if network else 'none') + '.efi'
        if args.debug:
            build.append('DEBUG=' + args.debug)
        if args.kind != 'selftests':
            script = menu
            if args.kind == 'input':
                script = script.replace('item test Continue test',
                                        'item wrong Wrong default\nitem test Continue test')
                script = script.replace('--default test --timeout 1500',
                                        '--default wrong --timeout 10000')
            elif network:
                url = 'http://10.0.2.2:' + str(server.server_port)
                script = ('#!ipxe\ndhcp || goto failed\nimgfetch --name payload ' +
                          url + '/payload.bin || goto failed\nsha256sum payload\n'
                          'imgfree payload\nimgfetch --name tftp_payload tftp://10.0.2.2/payload.bin || goto failed\n'
                          'sha256sum tftp_payload\nimgfree tftp_payload\nchain ' + url + '/menu.ipxe || goto failed\n'
                          'echo IA64_NETWORK_OK\nexit\n:failed\n'
                          'echo IA64_TEST_FAILED\nexit 1\n')
            if args.kind in ('snp-network', 'deployed-menu','linux'):
                stage = args.output / 'snp-stage.efi'
                if args.chain_efi:
                    shutil.copy2(args.chain_efi, stage)
                else:
                    stage_embed = args.output / 'snp.ipxe'
                    stage_embed.write_text(script.replace('IA64_NETWORK_OK', 'IA64_SNP_NETWORK_OK'))
                    stage_build = build + ['EMBED=' + str(stage_embed), 'bin-ia64-efi/snp.efi']
                    (args.output / 'stage-build-command.json').write_text(json.dumps(stage_build, indent=2) + '\n')
                    with (args.output / 'stage-build.log').open('wb') as log:
                        subprocess.run(stage_build, check=True, stdout=log, stderr=subprocess.STDOUT)
                    shutil.copy2(ROOT / 'src/bin-ia64-efi/snp.efi', stage)
                extra_files['/snp-stage.efi'] = stage.read_bytes()
                script = ('#!ipxe\ndhcp || goto failed\nchain ' + url +
                          '/snp-stage.efi || goto failed\necho IA64_CHAIN_RETURN_OK\nexit\n'
                          ':failed\necho IA64_TEST_FAILED\nexit 1\n')
            embed = args.output / 'embedded.ipxe'
            embed.write_text(script)
            build.append('EMBED=' + str(embed))
        else:
            build.append('EMBED=')
        build.append('bin-ia64-efi/' + target)
        (args.output / 'build-command.json').write_text(json.dumps(build, indent=2) + '\n')
        with (args.output / 'build.log').open('wb') as log:
            subprocess.run(build, check=True, stdout=log, stderr=subprocess.STDOUT)
        binary = args.output / target
        shutil.copy2(ROOT / 'src/bin-ia64-efi' / target, binary)
        disk = args.output / 'boot.fat'
        with disk.open('wb') as f:
            f.truncate(64 * 1024 * 1024)
        subprocess.run(['mkfs.vfat', '-F', '16', str(disk)], check=True,
                       stdout=subprocess.DEVNULL)
        subprocess.run(['mmd', '-i', str(disk), '::/EFI', '::/EFI/BOOT'], check=True)
        subprocess.run(['mcopy', '-i', str(disk), str(binary),
                        '::/EFI/BOOT/BOOTIA64.EFI'], check=True)
        firmware = args.output / 'firmware.bin'
        shutil.copy2(args.firmware, firmware)
        command = [args.qemu, '-machine', args.machine + ',nvram=none',
                   '-m', args.memory, '-bios', str(firmware),
                   '-drive', 'if=ide,format=raw,snapshot=on,file=' + str(disk),
                   '-nic', 'user,model=e1000,id=net0,tftp=' + str(args.output) if network else 'none',
                   '-display', 'none', '-serial', 'stdio', '-monitor',
                   'unix:' + str(args.output / 'monitor.sock') + ',server=on,wait=off',
                   '-no-reboot']
        if network:
            (args.output / 'payload.bin').write_bytes(payload)
            command += ['-object', 'filter-dump,id=capture,netdev=net0,file=' +
                        str(args.output / 'network.pcap')]
        if args.iso:
            command+=['-drive','if=ide,index=1,format=raw,media=cdrom,readonly=on,file='+str(args.iso)]
        if args.trace:
            command += ['-d', 'int,guest_errors', '-D', str(args.output / 'exceptions.log')]
        (args.output / 'command.json').write_text(json.dumps(command, indent=2) + '\n')
        passed = False
        sent = False
        serial = args.output / 'serial.log'
        with serial.open('wb') as log:
            def limit_trace():
                resource.setrlimit(resource.RLIMIT_FSIZE, (32 * 1024 * 1024, 32 * 1024 * 1024))
            proc = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=log,
                                    stderr=subprocess.STDOUT, preexec_fn=limit_trace if args.trace else None)
            end = time.monotonic() + args.seconds
            while time.monotonic() < end:
                raw = serial.read_bytes().decode(errors='replace')
                clean = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw)
                if args.kind == 'input' and 'Wrong default' in clean and not sent:
                    time.sleep(0.3)
                    proc.stdin.write(b'\x1b[B\r')
                    proc.stdin.flush()
                    sent = True
                if args.kind == 'selftests':
                    match = re.search(r'all (\d+) tests passed', clean, re.I)
                    passed = bool(match and int(match.group(1)) >= 12240)
                elif args.kind == 'linux':
                    if not sent and ('root@livecd' in clean or 'livecd ~ #' in clean):
                        proc.stdin.write(b"uname -m; printf '\\nQEMU_IA64_OS_RUNTIME_OK\\n'\r");proc.stdin.flush();sent=True
                    passed = (sent and 'Linux version ' in clean and
                              bool(re.search(r'(?:^|\n)QEMU_IA64_OS_RUNTIME_OK\r?\n',clean)) and
                              '/snp-stage.efi' in requests and
                              (not args.rootfs or '/root-verified' in requests))
                elif args.kind == 'deployed-menu':
                    passed = ('IA64 Netboot' in clean and 'IA64_CHAIN_RETURN_OK' in clean and
                              '/snp-stage.efi' in requests)
                elif network:
                    passed = (marker in clean and
                              ('IA64_NETWORK_OK' in clean if args.kind == 'network' else
                               'IA64_SNP_NETWORK_OK' in clean and 'IA64_CHAIN_RETURN_OK' in clean) and
                              clean.count(digest) >= 2 and '/payload.bin' in requests and
                              '/menu.ipxe' in requests)
                else:
                    passed = marker in clean and (args.kind != 'input' or sent)
                if (passed or 'IA64_TEST_FAILED' in clean or 'System halted.' in clean or
                    'FAIL:' in clean or proc.poll() is not None):
                    break
                time.sleep(0.25)
        result = dict(architecture='ia64', kind=args.kind, machine=args.machine,
                      passed=passed, http_requests=requests,
                      binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                      emulator_version=subprocess.check_output([args.qemu, '--version'],
                                                              text=True).splitlines()[0],
                      firmware_sha256=hashlib.sha256(firmware.read_bytes()).hexdigest(),
                      hardware_verified=False, firmware_network_boot_verified=False,
                      firmware_fat_boot_verified=passed,
                      operating_system_handoff_verified=passed and args.kind=='linux',
                      network_root_verified=passed and (bool(args.rootfs) or bool(args.expected_rootfs) and 'NETBOOT_ROOT_VERIFIED' in clean),
                      expected_root_sha256=hashlib.sha256(args.expected_rootfs.read_bytes()).hexdigest() if args.expected_rootfs else None,
                      console_input_verified=passed and args.kind == 'input',
                      snp_network_verified=passed and args.kind == 'snp-network',
                      efi_program_chain_verified=passed and args.kind in ('snp-network', 'deployed-menu'),
                      deployed_menu_verified=passed and args.kind == 'deployed-menu',
                      chained_efi_sha256=hashlib.sha256((args.output / 'snp-stage.efi').read_bytes()).hexdigest()
                          if args.kind in ('snp-network', 'deployed-menu','linux') else None)
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
