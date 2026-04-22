# -*- mode: python ; coding: utf-8 -*-
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from pathlib import Path
import sys


spec_dir = Path(SPECPATH).resolve()
sys.path.insert(0, str(spec_dir))

import bundle_dylibs


script_path = spec_dir / "qml_mcp_server.py"
binaries = []

if sys.platform == "darwin":
    # Python's _ssl/_hashlib reference libssl/libcrypto via @rpath,
    # which PyInstaller doesn't auto-collect for pyenv-style builds.
    binaries = bundle_dylibs.collected_binaries(["_ssl", "_hashlib"])


a = Analysis(
    [str(script_path)],
    pathex=[str(spec_dir)],
    binaries=binaries,
    datas=[],
    hiddenimports=[],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name="qml_mcp_server",
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    contents_directory="_internal",
)

coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name="qml_mcp_server",
)
