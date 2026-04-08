# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from __future__ import annotations
import argparse
import asyncio
from itertools import islice
import os
import locale
from pathlib import Path
import plistlib
import re
import shutil
import struct
import subprocess
import sys
from urllib.parse import urlparse
import urllib.request

encoding = locale.getdefaultlocale()[1]
if not encoding:
    encoding = 'UTF-8'

def is_windows_platform():
    return sys.platform.startswith('win')

def is_linux_platform():
    return sys.platform.startswith('linux')

def is_mac_platform():
    return sys.platform.startswith('darwin')

def to_posix_path(path):
    if is_windows_platform():
        # should switch to pathlib from python3
        return path.replace('\\', '/')
    return path

def check_print_call(command, cwd=None, env=None):
    print('------------------------------------------')
    print('COMMAND:')
    print(' '.join(['"' + c.replace('"', '\\"') + '"' for c in command]))
    print('PWD:      "' + (str(cwd) if cwd else os.getcwd()) + '"')
    print('------------------------------------------')
    subprocess.check_call(command, cwd=cwd, shell=is_windows_platform(), env=env)


def get_git_SHA(path):
    try:
        output = subprocess.check_output(['git', 'rev-list', '-n1', 'HEAD'], cwd=path).strip()
        decoded_output = output.decode(encoding) if encoding else output
        return decoded_output
    except subprocess.CalledProcessError:
        return None
    return None


# get commit SHA either directly from git, or from a .tag file in the source directory
def get_commit_SHA(path):
    git_sha = get_git_SHA(path)
    if not git_sha:
        tagfile = os.path.join(path, '.tag')
        if os.path.exists(tagfile):
            with open(tagfile, 'r') as f:
                git_sha = f.read().strip()
    return git_sha


def get_single_subdir(path: Path):
    entries = list(islice(path.iterdir(), 2))
    if len(entries) == 1:
        return path / entries[0]
    return path


def sevenzip_command(threads=None):
    # use -mf=off to avoid usage of the ARM executable compression filter,
    # which cannot be extracted by p7zip
    # use -snl to preserve symlinks even if their target doesn't exist
    # which is important for the _dev package on Linux
    # (only works with official/upstream 7zip)
    command = ['7z', 'a', '-mf=off', '-snl']
    if threads:
        command.extend(['-mmt' + threads])
    return command


# copy of shutil.copytree that does not bail out if the target directory already exists
# and that does not create empty directories
def copytree(src, dst, symlinks=False, ignore=None):
    def ensure_dir(destdir, ensure):
        if ensure and not os.path.isdir(destdir):
            os.makedirs(destdir)
        return False

    names = os.listdir(src)
    if ignore is not None:
        ignored_names = ignore(src, names)
    else:
        ignored_names = set()

    needs_ensure_dest_dir = True
    errors = []
    for name in names:
        if name in ignored_names:
            continue
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)
        try:
            if symlinks and os.path.islink(srcname):
                needs_ensure_dest_dir = ensure_dir(dst, needs_ensure_dest_dir)
                linkto = os.readlink(srcname)
                os.symlink(linkto, dstname)
            elif os.path.isdir(srcname):
                copytree(srcname, dstname, symlinks, ignore)
            else:
                needs_ensure_dest_dir = ensure_dir(dst, needs_ensure_dest_dir)
                shutil.copy2(srcname, dstname)
            # XXX What about devices, sockets etc.?
        except (IOError, os.error) as why:
            errors.append((srcname, dstname, str(why)))
        # catch the Error from the recursive copytree so that we can
        # continue with other files
        except shutil.Error as err:
            errors.extend(err.args[0])
    try:
        if os.path.exists(dst):
            shutil.copystat(src, dst)
    except shutil.WindowsError:
        # can't copy file access times on Windows
        pass
    except OSError as why:
        errors.extend((src, dst, str(why)))
    if errors:
        raise shutil.Error(errors)


def extract_file(archive: Path, target: Path) -> None:
    cmd_args = []
    if archive.suffix == '.tar':
        cmd_args = ['tar', '-xf', str(archive)]
    elif archive.suffixes[-2:] == ['.tar', '.gz'] or archive.suffix == '.tgz':
        cmd_args = ['tar', '-xzf', str(archive)]
    elif archive.suffixes[-2:] == ['.tar', '.xz']:
        cmd_args = ['tar', '-xf', str(archive)]
    elif archive.suffixes[-2:] == ['.tar', '.bz2'] or archive.suffix == '.tbz':
        cmd_args = ['tar', '-xjf', str(archive)]
    elif archive.suffix in ('.7z', '.zip', '.gz', '.xz', '.bz2', '.qbsp'):
        cmd_args = ['7z', 'x', str(archive)]
    else:
        raise(
            "Extract fail: %s. Not an archive or appropriate extractor was not found", str(archive)
        )
        return
    target.mkdir(parents=True, exist_ok=True)
    subprocess.check_call(cmd_args, cwd=target)


async def download(url: str, target: Path) -> None:
    print(('''
- Starting download {}
                 -> {}''').strip().format(url, str(target)))
    # Since urlretrieve does blocking I/O it would prevent parallel downloads.
    # Run in default thread pool.
    temp_target = target.with_suffix(target.suffix + '-part')
    loop = asyncio.get_running_loop()
    await loop.run_in_executor(None, urllib.request.urlretrieve, url, str(temp_target))
    temp_target.rename(target)
    print('+ finished downloading {}'.format(str(target)))


def download_and_extract(
    urls: list[str],
    target: Path,
    temp: Path,
    skip_existing: bool = False
) -> None:
    download_and_extract_tuples([(url, target) for url in urls],
                                temp,
                                skip_existing)


def download_and_extract_tuples(
    urls_and_targets: list[tuple[str, Path]],
    temp: Path,
    skip_existing: bool = False
) -> None:
    temp.mkdir(parents=True, exist_ok=True)
    target_tuples : list[tuple[Path, Path]] = []
    # TODO make this work with file URLs, which then aren't downloaded
    #      but just extracted
    async def impl():
        tasks : list[asyncio.Task] = []
        for (url, target_path) in urls_and_targets:
            u = urlparse(url)
            filename = Path(u.path).name
            target_file = temp / filename
            target_tuples.append((target_file, target_path))
            if skip_existing and target_file.exists():
                print('Skipping download of {}'.format(url))
            else:
                tasks.append(asyncio.create_task(download(url, target_file)))
        for task in tasks:
            await task
    asyncio.run(impl())
    for (file, target) in target_tuples:
        extract_file(file, target)


def get_qt_install_info(qmake_bin):
    output = subprocess.check_output([qmake_bin, '-query'])
    decoded_output = output.decode(encoding) if encoding else output
    lines = decoded_output.strip().split('\n')
    info = {}
    for line in lines:
        (var, sep, value) = line.partition(':')
        info[var.strip()] = value.strip()
    return info

def get_rpath(libfilepath, chrpath=None):
    if chrpath is None:
        chrpath = 'chrpath'
    try:
        output = subprocess.check_output([chrpath, '-l', libfilepath]).strip()
        decoded_output = output.decode(encoding) if encoding else output
    except subprocess.CalledProcessError: # no RPATH or RUNPATH
        return []
    marker = 'RPATH='
    index = decoded_output.find(marker)
    if index < 0:
        marker = 'RUNPATH='
        index = decoded_output.find(marker)
    if index < 0:
        return []
    return decoded_output[index + len(marker):].split(':')

def fix_rpaths(path, qt_deploy_path, qt_install_info, chrpath=None):
    if chrpath is None:
        chrpath = 'chrpath'
    qt_install_prefix = qt_install_info['QT_INSTALL_PREFIX']
    qt_install_libs = qt_install_info['QT_INSTALL_LIBS']

    def fix_rpaths_helper(filepath):
        rpath = get_rpath(filepath, chrpath)
        if len(rpath) <= 0:
            return
        # remove previous Qt RPATH
        new_rpath = [path for path in rpath if not path.startswith(qt_install_prefix)
                     and not path.startswith(qt_install_libs)]

        # check for Qt linking
        lddOutput = subprocess.check_output(['ldd', filepath])
        lddDecodedOutput = lddOutput.decode(encoding) if encoding else lddOutput
        if lddDecodedOutput.find('libQt') >= 0 or lddDecodedOutput.find('libicu') >= 0:
            # add Qt RPATH if necessary
            relative_path = os.path.relpath(qt_deploy_path, os.path.dirname(filepath))
            if relative_path == '.':
                relative_path = ''
            else:
                relative_path = '/' + relative_path
            qt_rpath = '$ORIGIN' + relative_path
            if not any((path == qt_rpath) for path in rpath):
                new_rpath.append(qt_rpath)

        # change RPATH
        if len(new_rpath) > 0:
            subprocess.check_call([chrpath, '-r', ':'.join(new_rpath), filepath])
        else: # no RPATH / RUNPATH left. delete.
            subprocess.check_call([chrpath, '-d', filepath])

    def is_unix_executable(filepath):
        # Whether a file is really a binary executable and not a script and not a symlink (unix only)
        if os.path.exists(filepath) and os.access(filepath, os.X_OK) and not os.path.islink(filepath):
            with open(filepath, 'rb') as f:
                return f.read(2) != "#!"

    def is_unix_library(filepath):
        # Whether a file is really a library and not a symlink (unix only)
        return os.path.basename(filepath).find('.so') != -1 and not os.path.islink(filepath)

    for dirpath, dirnames, filenames in os.walk(path):
        for filename in filenames:
            filepath = os.path.join(dirpath, filename)
            if is_unix_executable(filepath) or is_unix_library(filepath):
                fix_rpaths_helper(filepath)

def is_debug_file(filepath):
    if is_mac_platform():
        return filepath.endswith('.dSYM') or '.dSYM/' in filepath
    elif is_linux_platform():
        return filepath.endswith('.debug')
    else:
        return filepath.endswith('.pdb')

def is_debug(path, filenames):
    return [fn for fn in filenames if is_debug_file(os.path.join(path, fn))]

def is_not_debug(path, filenames):
    files = [fn for fn in filenames if os.path.isfile(os.path.join(path, fn))]
    return [fn for fn in files if not is_debug_file(os.path.join(path, fn))]

def codesign_call(identity=None, flags=None):
    signing_identity = identity or os.environ.get('SIGNING_IDENTITY')
    if not signing_identity:
        return None
    codesign_call = ['codesign', '-o', 'runtime', '--force', '-s', signing_identity,
                     '-v']
    signing_flags = flags or os.environ.get('SIGNING_FLAGS')
    if signing_flags:
        codesign_call.extend(['-r', signing_flags])
    return codesign_call


def _bundle_executable_name(app_bundle: Path) -> str | None:
    """Read CFBundleExecutable from the bundle Info.plist if available."""
    info_plist = app_bundle / "Contents" / "Info.plist"
    if not info_plist.is_file():
        return None
    try:
        with info_plist.open("rb") as handle:
            info = plistlib.load(handle)
    except (OSError, plistlib.InvalidFileException, ValueError):
        return None
    executable = info.get("CFBundleExecutable")
    return executable if isinstance(executable, str) and executable else None


def _source_entitlements_dir() -> Path:
    return Path(__file__).resolve().parent.parent / "dist" / "installer" / "mac"


def _entitlements_dirs(extra_entitlements_dir: str | Path | None = None) -> list[Path]:
    entitlements_dirs: list[Path] = []
    for candidate in (extra_entitlements_dir, _source_entitlements_dir()):
        if candidate is None:
            continue
        directory = Path(candidate)
        if directory.is_dir() and directory not in entitlements_dirs:
            entitlements_dirs.append(directory)
    return entitlements_dirs


def _entitlements_path_for_item(
    path: Path, extra_entitlements_dir: str | Path | None = None
) -> Path | None:
    if path.suffix == ".app":
        app_names = [path.stem]
        executable_name = _bundle_executable_name(path)
        if executable_name and executable_name not in app_names:
            app_names.append(executable_name)
        for entitlements_dir in _entitlements_dirs(extra_entitlements_dir):
            for app_name in app_names:
                entitlements_path = entitlements_dir / f"{app_name}.entitlements"
                if entitlements_path.is_file():
                    return entitlements_path
        return None

    for entitlements_dir in _entitlements_dirs(extra_entitlements_dir):
        entitlements_path = entitlements_dir / f"{path.name}.entitlements"
        if entitlements_path.is_file():
            return entitlements_path
    return None

def _is_mach_o_file(path: Path) -> bool:
    """Determine whether a file is a Mach-O image containing native code."""
    try:
        with path.open("rb") as f:
            header = f.read(8)
    except OSError:
        return False
    if len(header) < 4:
        return False
    magic = header[:4]
    if magic in {
        b"\xfe\xed\xfa\xce",  # MH_MAGIC
        b"\xce\xfa\xed\xfe",  # MH_CIGAM
        b"\xfe\xed\xfa\xcf",  # MH_MAGIC_64
        b"\xcf\xfa\xed\xfe",  # MH_CIGAM_64
    }:
        return True
    # Universal (fat) binary shares 0xCAFEBABE with Java class files.
    # Disambiguate by nfat_arch at offset 4 (< 20 means Mach-O).
    if len(header) >= 8 and magic in {b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"}:
        byte_order = ">" if magic == b"\xca\xfe\xba\xbe" else "<"
        nfat_arch = struct.unpack(f"{byte_order}I", header[4:8])[0]
        return 0 < nfat_arch < 20
    return False

def _is_framework_version(path: Path) -> bool:
    """Determine whether a path is a versioned macOS framework directory."""
    return path.parent.name == "Versions" and path.parent.parent.suffix == ".framework"

def _collect_signable_items(app_path: Path) -> list[Path]:
    """Collect all Mach-O files and bundles under app_path, sorted deepest-first."""
    items: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(app_path):
        dp = Path(dirpath)
        for dirname in dirnames:
            full = dp / dirname
            if full.suffix == ".app":
                items.append(full)
            elif full.suffix == ".framework":
                # Sign individual versions, not the framework bundle itself
                versions_dir = full / "Versions"
                if versions_dir.is_dir():
                    for entry in versions_dir.iterdir():
                        if entry.is_dir() and not entry.is_symlink():
                            items.append(entry)
        for filename in filenames:
            filepath = dp / filename
            if filepath.is_symlink():
                continue
            if filepath.suffix == ".dylib" or _is_mach_o_file(filepath):
                items.append(filepath)
    items.sort(key=lambda p: len(p.parts), reverse=True)
    return items

def _sign_item_with_deps(
    path: Path,
    codesign_cmd: list[str],
    app_path: Path,
    signed: set[Path],
    extra_entitlements_dir: str | Path | None = None,
):
    """Sign a single item, recursively signing unsigned dependencies first."""
    if path in signed:
        return
    cmd = list(codesign_cmd)
    entitlements_path = _entitlements_path_for_item(path, extra_entitlements_dir)
    if entitlements_path is not None and entitlements_path.exists():
        cmd.extend(["--entitlements", str(entitlements_path)])
    if path.suffix == ".app" or _is_framework_version(path):
        cmd.append("--preserve-metadata=entitlements")
    while True:
        result = subprocess.run([*cmd, str(path)], capture_output=True)
        if result.returncode == 0:
            signed.add(path)
            return
        output = result.stdout.decode(errors="ignore") + result.stderr.decode(errors="ignore")
        match = re.search(
            re.escape(str(path))
            + r": code object is not signed at all\nIn subcomponent: (.+)",
            output,
        )
        if match:
            dep_path = Path(match.group(1).strip())
            if not dep_path.is_relative_to(app_path):
                msg = f"Dependency {dep_path} is outside of app bundle {app_path}"
                raise RuntimeError(msg)
            if dep_path in signed:
                msg = f"Already signed {dep_path} but still failing for {path}"
                raise RuntimeError(msg)
            _sign_item_with_deps(dep_path, codesign_cmd, app_path, signed, extra_entitlements_dir)
            continue
        print(f"codesign failed (exit {result.returncode}) for: {path}", flush=True)
        print(f"  command: {' '.join(str(c) for c in cmd)} {path}", flush=True)
        if output.strip():
            print(f"  output:  {output.strip()}", flush=True)
        raise subprocess.CalledProcessError(
            result.returncode, [*cmd, str(path)], result.stdout, result.stderr,
        )


def codesign(app_path, identity=None, flags=None, entitlements_dir=None):
    codesign_cmd = codesign_call(identity, flags)
    if not codesign_cmd or not is_mac_platform():
        return
    root = Path(app_path).resolve()
    # Collect all signable items and sign bottom-up (deepest first)
    items = _collect_signable_items(root)
    signed: set[Path] = set()
    for item in items:
        _sign_item_with_deps(item, codesign_cmd, root, signed, entitlements_dir)
    # Sign the top-level bundle
    if root not in signed and root.suffix in (".app", ".framework"):
        _sign_item_with_deps(root, codesign_cmd, root, signed, entitlements_dir)



def codesign_main(args):
    codesign(args.app_bundle, args.identity, args.flags, args.entitlements_dir)

def main():
    parser = argparse.ArgumentParser(description='Qt Creator build tools')
    subparsers = parser.add_subparsers(title='subcommands', required=True)
    parser_codesign = subparsers.add_parser('codesign', description='Codesign macOS app bundle')
    parser_codesign.add_argument('app_bundle')
    parser_codesign.add_argument('-s', '--identity', help='Codesign identity')
    parser_codesign.add_argument('--flags', help='Additional flags')
    parser_codesign.add_argument('--entitlements-dir', help='Directory containing entitlements files')
    parser_codesign.set_defaults(func=codesign_main)
    args = parser.parse_args()
    args.func(args)

if __name__ == '__main__':
    main()
