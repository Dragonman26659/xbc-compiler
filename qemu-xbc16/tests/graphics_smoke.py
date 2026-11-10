#!/usr/bin/env python3
"""Exercise the actual graphical console and QMP input with the IRQ demo."""
import json
import queue
import struct
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
qemu = str(Path(sys.argv[1]).resolve())
binary = Path(__file__).resolve().parents[1] / 'examples/graphics.bin'
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    with (root / 'stderr.log').open('wb') as errors:
        proc = subprocess.Popen([qemu, '-M', 'xbc16', '-display', 'none',
                                 '-serial', 'none', '-monitor', 'none',
                                 '-kernel', str(binary), '-S', '-qmp', 'stdio'],
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=errors)
        messages = queue.Queue()
        def reader():
            for line in proc.stdout:
                messages.put(json.loads(line))
            messages.put({'error': 'QEMU exited before replying'})
        threading.Thread(target=reader, daemon=True).start()
        def command(name, arguments=None):
            request = {'execute': name}
            if arguments is not None:
                request['arguments'] = arguments
            proc.stdin.write((json.dumps(request) + '\n').encode())
            proc.stdin.flush()
            while True:
                response = messages.get(timeout=10)
                assert 'error' not in response, response
                if 'return' in response:
                    return response['return']
        def word(address):
            path = root / 'word.bin'
            command('pmemsave', {'val': address * 2, 'size': 2, 'filename': str(path)})
            return struct.unpack('<H', path.read_bytes())[0]
        def await_word(address, expected):
            end = time.monotonic() + 10
            actual = None
            while time.monotonic() < end:
                actual = word(address)
                if actual == expected:
                    return
                time.sleep(0.02)
            raise AssertionError(f'Word {address}: expected {expected:#06x}, last read {actual:#06x}')
        try:
            assert 'QMP' in messages.get(timeout=10)
            command('qmp_capabilities')
            command('cont')
            await_word(63724, 0x1e30)
            await_word(63961, 0x1e30)
            await_word(64029, 0x1f30)
            for index, (key, code) in enumerate([('w',0x11),('a',0x1e),('s',0x1f),('d',0x20)]):
                command('input-send-event', {'events': [
                    {'type': 'key', 'data': {'down': True, 'key': {'type': 'qcode', 'data': key}}}]})
                await_word(63799, 0x1f00 | ord(key))
                for offset, digit in enumerate(f'{code:04X}'):
                    await_word(63873 + offset, 0x1f00 | ord(digit))
                command('input-send-event', {'events': [
                    {'type': 'key', 'data': {'down': False, 'key': {'type': 'qcode', 'data': key}}}]})
                await_word(63873, 0x1f38)
                await_word(63799, 0x1f00 | ord(key))
                await_word(63724, 0x1e30 + (index + 1) * 2)
            command('input-send-event', {'events': [
                {'type': 'btn', 'data': {'down': True, 'button': 'left'}},
                {'type': 'rel', 'data': {'axis': 'x', 'value': -3}},
                {'type': 'rel', 'data': {'axis': 'y', 'value': 4}}]})
            await_word(63961, 0x1e31)
            await_word(64029, 0x1f31)
            image = root / 'screen.ppm'
            command('screendump', {'filename': str(image)})
            with image.open('rb') as f:
                assert f.readline().strip() == b'P6'
                assert f.readline().strip() == b'640 400'
                assert f.readline().strip() == b'255'
                pixels = f.read()
            state = command('human-monitor-command', {'command-line': 'info registers'})
            assert 'R16=0xf72c' in state, state  # Fully balanced 15-word frames.
            assert len(pixels) == 640 * 400 * 3
            assert pixels[-3:] == b'\x00\x00\xaa'
            assert b'\xff\xff\xff' in pixels
            command('quit')
            assert proc.wait(timeout=10) == 0
        finally:
            if proc.poll() is None:
                proc.kill()
                proc.wait(timeout=10)
            proc.stdin.close()
            proc.stdout.close()
print('QEMU VGA text screendump and keyboard/mouse IRQ demo checks passed')
