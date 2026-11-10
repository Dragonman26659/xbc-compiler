# SPDX-License-Identifier: GPL-2.0-or-later
import importlib.util
from pathlib import Path
import tempfile
import unittest
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('windows_package',root/'windows/package.py')
package=importlib.util.module_from_spec(spec);spec.loader.exec_module(package)
class DependencyTests(unittest.TestCase):
    def test_recursive_case_insensitive_graph_with_cycle_and_system_imports(self):
        with tempfile.TemporaryDirectory(prefix='dll test ') as folder:
            root=Path(folder);exe=root/'qemu.exe';exe.write_bytes(b'MZ')
            for name in ('libA.dll','libB.dll'): (root/name).write_bytes(b'MZ')
            graph={'qemu.exe':['KERNEL32.dll','LIBA.DLL','api-ms-win-crt-heap-l1-1-0.dll'],
                   'libA.dll':['libB.dll'], 'libB.dll':['liba.dll','USER32.dll']}
            dlls=package.collect(exe,root,lambda p:graph[p.name])
            self.assertEqual([p.name for p in dlls],['libA.dll','libB.dll'])
    def test_missing_dll_is_an_error(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);exe=root/'qemu.exe';exe.write_bytes(b'MZ')
            with self.assertRaises(FileNotFoundError):package.collect(exe,root,lambda p:['missing.dll'])
    def test_imports_cannot_escape_runtime_folder(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);exe=root/'qemu.exe';exe.write_bytes(b'MZ')
            with self.assertRaises(ValueError):package.collect(exe,root,lambda p:['../outside.dll'])
if __name__=='__main__':unittest.main()
