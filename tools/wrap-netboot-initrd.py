#!/usr/bin/env python3
"""Append the network-root prefix to an untouched official Linux initramfs."""
import argparse
import gzip
import hashlib
import json
import shlex
import stat
from pathlib import Path


def newc(name, data, mode=0o100755):
    name = name.encode() + b'\0'
    fields = [1, mode, 0, 0, 1, 0, len(data), 0, 0, 0, 0, len(name), 0]
    entry = b'070701' + b''.join(f'{n:08x}'.encode() for n in fields) + name
    entry += b'\0' * (-len(entry) % 4)
    entry += data
    return entry + b'\0' * (-len(data) % 4)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('official', type=Path)
    p.add_argument('output', type=Path)
    p.add_argument('--root-url', required=True)
    p.add_argument('--sha256', required=True)
    p.add_argument('--report-url', default='')
    p.add_argument('--overlay', type=Path)
    a = p.parse_args()
    original = a.official.read_bytes()
    prefix = Path(__file__).with_name('netboot-init').read_bytes()
    config = ''.join(k+'='+shlex.quote(v)+'\n' for k,v in
                     [('url',a.root_url),('digest',a.sha256),('report',a.report_url)])
    archive = newc('netboot-init', prefix) + newc('netboot.conf', config.encode(), 0o100644)
    if a.overlay:
        for entry in sorted(a.overlay.rglob('*')):
            mode=entry.lstat().st_mode
            if stat.S_ISREG(mode): data=entry.read_bytes()
            elif stat.S_ISDIR(mode): data=b''
            elif stat.S_ISLNK(mode): data=entry.readlink().as_posix().encode()
            else: raise ValueError('Unsupported overlay entry: '+str(entry))
            archive+=newc(entry.relative_to(a.overlay).as_posix(),data,mode)
    archive += newc('TRAILER!!!', b'', 0)
    archive += b'\0' * (-len(archive) % 512)
    # The kernel accepts concatenated compressed initramfs members.
    # Padding also handles archives whose compressed member is not aligned.
    result = original + b'\0' * (-len(original) % 4) + gzip.compress(archive, mtime=0)
    with a.output.open('xb') as f:
        f.write(result)
    print(json.dumps(dict(official_sha256=hashlib.sha256(original).hexdigest(),
                         prefix_sha256=hashlib.sha256(prefix).hexdigest(),
                         output_sha256=hashlib.sha256(result).hexdigest())))


if __name__ == '__main__':
    main()
