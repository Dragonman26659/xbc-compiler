#!/usr/bin/env python3
"""Compare the C target core against the previously updated C++ simulator."""
import argparse, random, re, struct, subprocess, tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('reference_sim');args=p.parse_args()
root=Path(__file__).resolve().parents[1]
rng=random.Random(16016)
subprocess.run(['cc','-std=c11','-O2',str(root/'tests/core_runner.c'),str(root/'overlay/target/xbc16/core.c'),'-lm','-o',str(root/'core-runner')],check=True)
with tempfile.TemporaryDirectory() as directory:
    for case in range(100):
        words=[]
        for r in range(16):
            if r==15:continue
            words.extend([r,rng.randrange(65536)])
        for i in range(100):
            a,b=rng.randrange(4),rng.randrange(4)
            op=rng.choice([0x80,0x40,0xc0,0x20,0x60,0xa0,0x61,0xa1,0xf8,0xe4,0xf800,0xfe00,0xfc00,0xf200,0xfa00,0xf900])
            if op>=0xf000:words.append(op|(a<<4)|b)
            elif op in [0xf8,0xe4]:words.append(op|a)
            elif op in [0xa0,0x61,0xa1]:words.append(op|(a<<1))
            else:words.append(op|(a<<3)|(b<<1))
        words.append(65535)
        binary=Path(directory)/'case.bin';binary.write_bytes(struct.pack('<'+'H'*len(words),*words))
        reference=subprocess.run([args.reference_sim,str(binary),'16'],capture_output=True,text=True,check=True).stdout
        regs=[int(x,16) for x in re.findall(r'^(?:EAX|EBX|ECX|EDX|r\d+|ACC)\s+0x([0-9A-F]+)',reference,re.M)]
        pc=int(re.search(r'^PC: (\d+)',reference,re.M)[1])
        flags=[int(x) for x in re.search(r'Flags: Z=(\d) N=(\d) U=(\d)',reference).groups()]
        actual=list(map(int,subprocess.check_output([str(root/'core-runner'),str(binary)],text=True).split()))
        assert actual==regs+[pc]+flags,(case,actual,regs+[pc]+flags)
print('100 randomized differential programs passed')
