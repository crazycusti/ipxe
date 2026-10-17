#!/usr/bin/env python3
"""Extract selected official modules and their dependencies from a live root.

Use the kernel release matching the original initramfs.  Module bytes and
indexes are taken unchanged from the authenticated official SquashFS.
"""
import argparse
from pathlib import Path
import subprocess


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('squashfs',type=Path)
    p.add_argument('output',type=Path)
    p.add_argument('--release',required=True)
    p.add_argument('modules',nargs='+')
    a=p.parse_args()
    if a.output.exists():p.error('Output already exists')
    lines=subprocess.check_output(['unsquashfs','-ll',str(a.squashfs)],text=True).splitlines()
    paths=[line.split()[-1].removeprefix('squashfs-root/') for line in lines if line.startswith('-')]
    depfile=next(path for path in paths if path.endswith('/'+a.release+'/modules.dep'))
    base=Path(depfile).parent.as_posix()
    deps={line.split(':')[0]:line.split(':')[1].strip().split() for line in
          subprocess.check_output(['unsquashfs','-cat',str(a.squashfs),depfile],text=True).splitlines()}
    selected=set()
    def add(path):
        if path in selected:return
        selected.add(path)
        for dependency in deps[path]:add(dependency)
    for name in a.modules:
        found=[path for path in deps if Path(path).name.removesuffix('.xz').removesuffix('.zst').removesuffix('.ko').replace('-','_')==name.replace('-','_')]
        if len(found)!=1:p.error('Expected one module '+name+', found '+str(len(found)))
        add(found[0])
    files=[base+'/'+path for path in sorted(selected)]+[path for path in paths if Path(path).parent.as_posix()==base]
    subprocess.run(['unsquashfs','-no-progress','-d',str(a.output),str(a.squashfs),*files],check=True,stdout=subprocess.DEVNULL)
    print('\n'.join(sorted(selected)))


if __name__=='__main__':main()
