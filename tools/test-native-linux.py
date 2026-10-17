#!/usr/bin/env python3
"""Fetch a real Linux ELF/initrd through iPXE, enter it and prove guest execution.

All guests are disposable, with user networking and a read-only live ISO.
No host disk, LAN interface or installation target is passed to QEMU.
"""
import argparse
import hashlib
import http.server
import json
from pathlib import Path
import re
import shutil
import signal
import socket
import struct
import subprocess
import threading
import time
ROOT = Path(__file__).resolve().parents[1]

def terminate(signum, _frame):
 raise SystemExit(128 + signum)

def main():
 signal.signal(signal.SIGTERM, terminate)
 p = argparse.ArgumentParser(description=__doc__)
 p.add_argument('architecture', choices=['alpha','powerpc','powerpc64','powerpc64le','hppa','sparc64'])
 p.add_argument('output', type=Path)
 p.add_argument('--kernel', type=Path, required=True)
 p.add_argument('--initrd', type=Path, required=True)
 p.add_argument('--iso', type=Path, required=True)
 p.add_argument('--args', required=True)
 p.add_argument('--qemu-dir', type=Path, default=Path('/opt/qemu/11.1.2/bin'))
 p.add_argument('--machine')
 p.add_argument('--seconds', type=int, default=420)
 p.add_argument('--debug')
 p.add_argument('--graphical-console', action='store_true', help='Prove Mac live console through framebuffer and keyboard')
 p.add_argument('--pci-serial', action='store_true', help='Add PCI UART for Linux on Mac99')
 a = p.parse_args()
 for source in (a.kernel, a.initrd, a.iso):
  if not source.is_file():p.error('Missing input: '+str(source))
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 requests=[]
 class Handler(http.server.BaseHTTPRequestHandler):
  def do_GET(self):
   source={'/kernel':a.kernel,'/initrd':a.initrd}.get(self.path)
   if source is None:self.send_error(404);return
   self.send_response(200);self.send_header('Content-Length',str(source.stat().st_size));self.end_headers()
   h=hashlib.sha256();count=0
   with source.open('rb') as f:
    while chunk:=f.read(1024*1024):self.wfile.write(chunk);h.update(chunk);count+=len(chunk)
   requests.append({'path':self.path,'sha256':h.hexdigest(),'bytes':count})
  def log_message(self,*_):pass
 server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
 threading.Thread(target=server.serve_forever,daemon=True).start()
 proc=None;uart=None
 try:
  url='http://10.0.2.2:'+str(server.server_port)
  script=a.output/'boot.ipxe';script.write_text('#!ipxe\ndhcp || goto failed\nkernel '+url+'/kernel '+a.args+' || goto failed\ninitrd --name initrd '+url+'/initrd || goto failed\nboot || goto failed\n:failed\necho NATIVE_OS_BOOT_FAILED\nshell\n')
  cross={'alpha':'/data/netboot/sources/multiarch-toolchains/gcc-13.3.0-nolibc/alpha-linux/bin/alpha-linux-','powerpc':'powerpc-linux-gnu-','powerpc64':'/data/netboot/sources/multiarch-toolchains/gcc-13.3.0-nolibc/powerpc64-linux/bin/powerpc64-linux-','powerpc64le':'powerpc64le-linux-gnu-','hppa':'hppa-linux-gnu-','sparc64':'sparc64-linux-gnu-'}[a.architecture]
  platform={'alpha':'srm','hppa':'pdc'}.get(a.architecture,'openfirmware')
  ext={'alpha':'srm','hppa':'lif'}.get(a.architecture,'mac' if a.machine=='mac99' and a.architecture=='powerpc64' else 'of')
  target=f'bin-{a.architecture}-{platform}/ipxe.{ext}'
  build=['make','-C',str(ROOT/'src'),'-j8','CROSS='+cross,'EMBED='+str(script),target]
  if a.debug:build.append('DEBUG='+a.debug)
  (a.output/'build-command.json').write_text(json.dumps(build,indent=2)+'\n')
  with (a.output/'build.log').open('wb') as f:subprocess.run(build,stdout=f,stderr=subprocess.STDOUT,check=True)
  binary=a.output/('ipxe.'+ext);shutil.copy2(ROOT/'src'/target,binary)
  if a.architecture=='alpha':
   command=[str(a.qemu_dir/'qemu-system-alpha'),'-M',a.machine or 'clipper','-m','1024','-kernel',str(binary),'-nic','user,model=e1000','-device','virtio-scsi-pci,id=scsi0','-drive','file='+str(a.iso)+',if=none,format=raw,media=cdrom,id=cd0,readonly=on','-device','scsi-cd,drive=cd0']
  elif a.architecture=='hppa':
   command=[str(a.qemu_dir/'qemu-system-hppa'),'-M',a.machine or 'B160L','-m','1024','-drive','if=scsi,format=raw,readonly=on,file='+str(binary),'-nic','user,model=e1000','-cdrom',str(a.iso)]
  elif a.architecture=='sparc64':
   command=[str(a.qemu_dir/'qemu-system-sparc64'),'-M',a.machine or 'sun4u','-m','2048','-kernel',str(binary),'-nic','user,model=e1000','-cdrom',str(a.iso)]
  elif a.machine=='mac99':
   image=bytearray(binary.read_bytes());off=struct.unpack_from('>I',image,28)[0];size,count=struct.unpack_from('>HH',image,42)
   for i in range(count):
    at=off+i*size
    if struct.unpack_from('>I',image,at)[0]==1:
     physical=struct.unpack_from('>I',image,at+12)[0];struct.pack_into('>I',image,at+12,physical-0x01000000)
   fixture=a.output/'mac99-kernel.elf';fixture.write_bytes(image)
   command=[str(a.qemu_dir/'qemu-system-ppc64'),'-M','mac99','-cpu','970' if a.architecture=='powerpc64' else 'G4','-m','2048','-kernel',str(fixture),'-nic','user,model=e1000','-cdrom',str(a.iso),'-prom-env','output-device=scca','-prom-env','input-device=scca']
  else:
   command=[str(a.qemu_dir/'qemu-system-ppc64'),'-M','pseries','-cpu','POWER9','-m','2048','-netdev','user,id=net0,tftp='+str(a.output)+',bootfile='+binary.name,'-device','virtio-net-pci,netdev=net0,bootindex=1','-prom-env','boot-device=net','-cdrom',str(a.iso)]
  if a.pci_serial:
   command+=['-chardev','socket,id=uart,path='+str(a.output/'uart.sock')+',server=on,wait=off','-device','pci-serial,chardev=uart']
  command+=['-nographic','-monitor','unix:'+str(a.output/'monitor.sock')+',server=on,wait=off','-no-reboot']
  (a.output/'command.json').write_text(json.dumps(command,indent=2)+'\n')
  serial=a.output/'serial.log';sent=False;runtime=False;linux=False;last_screen=0
  def monitor(command):
   with socket.socket(socket.AF_UNIX,socket.SOCK_STREAM) as m:
    m.settimeout(2);m.connect(str(a.output/'monitor.sock'));m.recv(65536);m.sendall((command+'\n').encode());return m.recv(65536)
  def keyboard(text):
   names={' ':'spc','-':'minus',';':'semicolon',':':'shift-semicolon','/':'slash','.':'dot','_':'shift-minus','\n':'ret'}
   for ch in text:
    monitor('sendkey '+names.get(ch,'shift-'+ch.lower() if ch.isupper() else ch));time.sleep(.08)
  with serial.open('wb') as f:
   proc=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=f,stderr=subprocess.STDOUT)
   end=time.monotonic()+a.seconds
   while time.monotonic()<end and proc.poll() is None:
    time.sleep(.25)
    if a.pci_serial:
     if uart is None and (a.output/'uart.sock').exists():
      uart=socket.socket(socket.AF_UNIX,socket.SOCK_STREAM);uart.connect(str(a.output/'uart.sock'));uart.setblocking(False)
     if uart is not None:
      try:
       chunk=uart.recv(65536)
       if chunk:
        with (a.output/'uart.log').open('ab') as u:u.write(chunk)
      except BlockingIOError:pass
    screen_text=''
    if a.graphical_console and time.monotonic()-last_screen>10 and (a.output/'monitor.sock').exists():
     last_screen=time.monotonic();monitor('screendump '+json.dumps(str(a.output/'screen.ppm')))
     if (a.output/'screen.ppm').exists():
      screen_text=subprocess.run(['tesseract',str(a.output/'screen.ppm'),'stdout','--psm','6'],capture_output=True,text=True).stdout
      (a.output/'screen.txt').write_text(screen_text)
    text=serial.read_text(errors='replace')
    if (a.output/'uart.log').exists():text+='\n'+(a.output/'uart.log').read_text(errors='replace')
    plain=re.sub(r'\x1b\[[0-9;?]*[A-Za-z]','',text).replace('\r','')
    linux='Linux version ' in plain and ('Linux OF: entry=' in plain or 'Linux Alpha: entry=' in plain or 'Linux PDC: entry=' in plain or 'Linux SPARC:' in plain)
    if a.graphical_console and not sent and ('root@livecd' in screen_text or 'livecd ~ #' in screen_text):
     keyboard('uname -m; echo QEMU_OS_RUNTIME_OK\n');sent=True
    if not a.graphical_console and not sent and ('root@livecd' in plain or 'livecd ~ #' in plain or ('Thank you for using Gentoo Linux' in plain and '(none) ~ #' in plain)):
     (uart.sendall if uart is not None else proc.stdin.write)(b"uname -m; printf '\\nQEMU_OS_RUNTIME_OK\\n'\r");proc.stdin.flush();sent=True
    runtime=bool(re.search(r'(?:^|\n)QEMU_OS_RUNTIME_OK\n',plain+'\n'+screen_text))
    if runtime or 'NATIVE_OS_BOOT_FAILED' in plain or 'Kernel panic' in plain:break
  passed=sent and linux and runtime and {'/kernel','/initrd'}<={r['path'] for r in requests}
  result={'architecture':a.architecture,'passed':passed,'linux_kernel_entered':linux,'guest_command_executed':runtime,'http_requests':requests,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'qemu_version':subprocess.check_output([command[0],'--version'],text=True).splitlines()[0],'iso_path':str(a.iso),'machine':a.machine,'guest_machine':next(iter(re.findall(r'(?:^|\n)(alpha|ppc|ppc64|ppc64le|sparc64|parisc|parisc64)\n',plain+'\n'+screen_text)),None),'guest_kernel_warnings':sorted(set(re.findall(r'BUG: [^\r\n]+',plain))),'iso_readonly':True,'root_filesystem_source':'read-only official live ISO','hardware_verified':False}
  (a.output/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
  if not passed:raise SystemExit(1)
 finally:
  if uart is not None:uart.close()
  if proc:
   proc.terminate()
   try:proc.wait(5)
   except subprocess.TimeoutExpired:proc.kill();proc.wait()
  server.shutdown();server.server_close()
if __name__=='__main__':main()
