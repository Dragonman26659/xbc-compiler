#!/usr/bin/env python3
"""Install the XBC16 target into an unmodified QEMU v9.2.0 source tree."""
from pathlib import Path
import argparse, shutil
p=argparse.ArgumentParser();p.add_argument('qemu_source',type=Path);args=p.parse_args()
root=args.qemu_source.resolve();bundle=Path(__file__).resolve().parent
if not (root/'VERSION').exists() or (root/'VERSION').read_text().strip() != '9.2.0':
    p.error('This extension is pinned to QEMU 9.2.0; use its v9.2.0 tag.')
changes={
 'target/meson.build': ("subdir('avr')", "subdir('xbc16')"),
 'hw/meson.build': ("subdir('avr')", "subdir('xbc16')"),
 'target/Kconfig': ('source avr/Kconfig', 'source xbc16/Kconfig'),
 'hw/Kconfig': ('source avr/Kconfig', 'source xbc16/Kconfig'),
 'include/sysemu/arch_init.h': ('    QEMU_ARCH_AVR = (1 << 21),', '    QEMU_ARCH_XBC16 = (1 << 24),'),
}
# The Windows-only raw-binary XBC16 board has no runtime install-tree assets.
# QEMU's development symlink bundle otherwise requires Windows symlink rights.
meson_file=root/'meson.build'
meson_text=meson_file.read_text()
bundle_marker='# XBC16: skip the optional Windows development symlink bundle.'
bundle_call="meson.add_postconf_script(find_program('scripts/symlink-install-tree.py'))"
bundle_anchor="target_dirs = config_host['TARGET_DIRS'].split()"
if bundle_marker not in meson_text:
    if meson_text.count(bundle_call)!=1 or bundle_anchor not in meson_text:
        p.error('meson.build: expected QEMU 9.2.0 install-tree anchors missing')
    meson_text=meson_text.replace(bundle_call+'\n','',1)
    block=(bundle_anchor+'\n\n'+bundle_marker+'\n'
           "if host_os != 'windows' or target_dirs != ['xbc16-softmmu']\n"
           '  '+bundle_call+'\n'
           "else\n  message('XBC16 Windows: development symlink bundle disabled')\nendif")
    meson_text=meson_text.replace(bundle_anchor,block,1)

# Check every anchor before changing the tree. Re-applying is idempotent.
for path,(anchor,new) in changes.items():
    text=(root/path).read_text()
    if new not in text and anchor not in text: p.error(f'{path}: expected QEMU 9.2.0 anchor missing')
for file in (bundle/'overlay').rglob('*'):
    if file.is_file():
        dest=root/file.relative_to(bundle/'overlay');dest.parent.mkdir(parents=True,exist_ok=True)
        if not dest.exists() or dest.read_bytes()!=file.read_bytes(): shutil.copyfile(file,dest)
for path,(anchor,new) in changes.items():
    target=root/path;text=target.read_text()
    if new not in text: target.write_text(text.replace(anchor,anchor+'\n'+new,1))
if meson_file.read_text()!=meson_text: meson_file.write_text(meson_text)
print('Installed native xbc16-softmmu target and xbc16 machine.')
