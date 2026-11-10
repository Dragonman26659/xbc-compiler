#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Make QEMU's forced-offline initial dependency check work with MinGW Python."""
import argparse
from pathlib import Path
import subprocess
import sys


def ensure_pycotap(source, target):
    target.mkdir(parents=True,exist_ok=True)
    # Prefer the exact wheel tracked in the pinned upstream checkout. Passing a
    # filesystem path avoids the MinGW pip file://C:/ find-links URL problem.
    wheel=source/'python/wheels/pycotap-1.3.1-py3-none-any.whl'
    requirement=str(wheel) if wheel.is_file() else 'pycotap==1.3.1'
    marker=target/'pycotap-1.3.1.dist-info/METADATA'
    module=target/'pycotap/__init__.py'
    if not (marker.is_file() and module.is_file()):
        print('Preparing pycotap 1.3.1 in private build dependencies...',flush=True)
        subprocess.run([sys.executable,'-m','pip','--disable-pip-version-check',
                        'install','--upgrade','--no-deps','--target',str(target),requirement],check=True)
    subprocess.run([sys.executable,'-c',
                    'import sys;sys.path.insert(0,sys.argv[1]);'
                    'import pycotap;import importlib.metadata as m;'
                    'assert m.version("pycotap")=="1.3.1";print("pycotap 1.3.1 ready")',str(target)],check=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--target',type=Path,required=True)
    args=parser.parse_args()
    ensure_pycotap(args.source.resolve(),args.target.resolve())

if __name__=='__main__':main()
