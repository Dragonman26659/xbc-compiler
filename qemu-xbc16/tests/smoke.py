#!/usr/bin/env python3
"""Actual QEMU binary-boot, memory, float, trap and SMC checks."""
import argparse, struct, subprocess, tempfile, json, os
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('qemu');p.add_argument('--isolated-runtime',action='store_true');args=p.parse_args()
qemu=str(Path(args.qemu).resolve())
run_env=os.environ.copy()
if args.isolated_runtime:
    if os.name!='nt':p.error('--isolated-runtime is for Windows DLL validation')
    system=Path(run_env.get('SystemRoot','C:\\Windows'))
    run_env['PATH']=str(Path(qemu).parent)+';'+str(system/'System32')
with tempfile.TemporaryDirectory() as directory:
    def boot(words, expected=None, ok=True):
        binary=Path(directory)/'boot.bin'
        binary.write_bytes(struct.pack('<'+'H'*len(words),*words))
        result=subprocess.run([qemu,'-M','xbc16,exit-on-halt=on','-display','none','-serial','none','-monitor','none','-kernel',str(binary)],capture_output=True,text=True,timeout=10,env=run_env)
        assert (result.returncode==0)==ok,result.stdout+result.stderr
        if expected:
            for value in expected: assert value in result.stdout,result.stdout+result.stderr
        return result
    boot([0,40,1,2,0x82,0xa1,0xec,0xf6,0xffff],['R1=0x002a','R3=0x002a','R16=0xf72c','HLT=1'])
    boot([0,0x3e00,1,0x4000,0xfa01,0xa1,0xf302,0xffff],['R1=0x4200','R3=0x0003'])
    # Store into ordinary unified RAM; bytes convert from word addresses.
    boot([0,123,1,100,0x63,0xe8,0xf2,0xffff],['R3=0x007b'])
    # Fault-vector setup in a complete RAM image, then unsigned divide by zero.
    image=[0]*65536;image[0]=0;image[1]=42;image[2]=0xf401;image[100]=65535;image[63284]=100
    boot(image,['R1=0x0006','R16=0xf72a','PC(word)=100'])
    image=[0]*65536;image[0]=0xe2;image[100]=65535;image[63280]=100;image[65535]=1
    boot(image,['PC(word)=100','R16=0xf71d'])
    # Execute a TB, overwrite its first word, and branch into it again.
    image=[0]*23
    image[:12]=[2,5,3,20,0x87,0,65535,1,20,0x63,0xe8,0x87]
    image[20:]=[0,1,0x85]
    boot(image,['PC(word)=20','R1=0xffff'])
    # Malformed raw binaries must be rejected before the CPU starts.
    binary=Path(directory)/'odd.bin';binary.write_bytes(b'\x00')
    result=subprocess.run([qemu,'-display','none','-serial','none','-monitor','none','-kernel',str(binary)],capture_output=True,text=True,timeout=10,env=run_env)
    assert result.returncode!=0 and 'complete little-endian words' in result.stderr,result.stderr
    # Exercise QEMU's actual input frontend through QMP, with no display.
    for vector,events,expected in [
        (0,[{'type':'key','data':{'down':True,'key':{'type':'number','data':65}}}],['R1=0x0041']),
        (1,[{'type':'btn','data':{'down':True,'button':'left'}},
            {'type':'rel','data':{'axis':'x','value':-3}},
            {'type':'rel','data':{'axis':'y','value':4}}],['R1=0x0001','R2=0xfffd','R3=0x0004'])]:
        image=[0]*65536;image[0]=0xe2;image[1]=0x81;image[100]=65535;image[63278+vector]=100
        binary=Path(directory)/'input.bin';binary.write_bytes(struct.pack('<65536H',*image))
        proc=subprocess.Popen([qemu,'-M','xbc16,exit-on-halt=on','-display','none','-serial','none',
                               '-monitor','none','-kernel',str(binary),'-S','-qmp','stdio'],
                               stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,bufsize=0,env=run_env)
        captured=[]
        try:
            assert 'QMP' in json.loads(proc.stdout.readline())
            def command(name,arguments=None):
                request={'execute':name}
                if arguments is not None:request['arguments']=arguments
                proc.stdin.write((json.dumps(request)+'\n').encode());proc.stdin.flush()
                while True:
                    line=proc.stdout.readline()
                    if not line:return
                    captured.append(line)
                    try:response=json.loads(line)
                    except json.JSONDecodeError:continue
                    if 'return' in response:return
                    if 'error' in response:raise AssertionError(response)
            command('qmp_capabilities');command('cont');command('input-send-event',{'events':events})
            output,error=proc.communicate(timeout=10)
            output=(b"".join(captured)+output).decode();error=error.decode()
            assert proc.returncode==0,output+error
            for value in expected:assert value in output,output+error
        finally:
            if proc.poll() is None:proc.kill();proc.communicate()
print('QEMU boot, RAM, float, timer, keyboard/mouse, interrupt, SMC and loader checks passed')
