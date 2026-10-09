#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Build the standalone qtprofiler for Qt for WebAssembly.

Drives the qtprofiler-wasm CMake preset, which cross-compiles the trace viewer
against a Qt for WebAssembly installation. Everything the preset needs is taken
from the command line, so a CI job needs no prepared environment: point it at an
emsdk, a Qt for WebAssembly and the matching host Qt and it configures, builds
and collects the deployable files.

With --build-qt it builds that Qt for WebAssembly first, configured and patched
as src/tools/qtprofiler/README.md describes, so that only an emsdk and the host
Qt have to be there beforehand.

The cache variables stay in CMakePresets.json rather than being repeated here,
so the preset remains the one description of what this build is.
"""

from __future__ import annotations

import argparse
import asyncio
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import common

# The preset that describes the build; see CMakePresets.json.
PRESET = 'qtprofiler-wasm'
TARGET = 'qtprofiler'

# What the WebAssembly finalizer emits into the build's "qtprofiler" directory.
# The application cannot be served without them, so a build that produced no
# error but not these has failed in a way worth reporting.
REQUIRED_ARTIFACTS = ['qtprofiler.html', 'qtprofiler.js', 'qtprofiler.wasm', 'qtloader.js']

# The Qt --build-qt downloads, which the patches in src/tools/qtprofiler/qt-patches
# are made for, and the checksums of its source packages. The Dockerfile next to
# the patches downloads the same.
QT_VERSION = '6.11.2'
QT_SOURCE_SHA256 = {
    'qtbase': '5b2e00eccaf5a4d8c14134ffa0ea8dfd0a35ae1ffc7f8d87fa4305a1ed23cf22',
    'qtsvg': 'd594337feca84c26fb67fe87b85e6a5c12fda404b611d905f9d138210c311876',
}

# What --build-qt writes into the Qt it installs: how that Qt was built, to tell
# whether it has to be built again, and that the directory is this script's to
# replace.
QT_STAMP = 'qtprofiler-qt-build.json'


def source_root() -> Path:
    return Path(__file__).resolve().parent.parent


def qtprofiler_sources(src: Path) -> Path:
    return src / 'src' / 'tools' / 'qtprofiler'


def existing_directory(value: str) -> Path:
    path = Path(value).expanduser()
    if not path.is_dir():
        raise argparse.ArgumentTypeError(f'no such directory: {path}')
    return path.resolve()


def get_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Build the standalone qtprofiler for Qt for WebAssembly.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f'''
examples:
  %(prog)s --emsdk ~/emsdk \\
      --qt-wasm ~/Qt/{QT_VERSION}/wasm_singlethread \\
      --qt-host ~/Qt/{QT_VERSION}/macos \\
      --output artifacts --zip qtprofiler-wasm.7z

  %(prog)s --build-qt --emsdk ~/emsdk \\
      --qt-host ~/Qt/{QT_VERSION}/macos \\
      --output artifacts

Each path also has an environment variable it falls back to, so an already
activated emsdk and a preset environment work without arguments.
'''.strip())

    parser.add_argument('--qt-wasm', type=Path,
                        help='Qt for WebAssembly installation to build against: a JSPI build '
                             'configured as src/tools/qtprofiler/README.md describes, or a '
                             'prebuilt single-threaded package such as '
                             f'~/Qt/{QT_VERSION}/wasm_singlethread. With --build-qt, where to '
                             'install the Qt it builds (default: <qt-build>/install). Otherwise '
                             'defaults to $QT_WASM_ROOT.')
    parser.add_argument('--qt-host', type=existing_directory,
                        default=os.environ.get('QT_HOST_PATH'),
                        help='Host Qt of the same version, whose tools run during the '
                             f'cross build, e.g. ~/Qt/{QT_VERSION}/macos. Defaults to '
                             '$QT_HOST_PATH.')
    parser.add_argument('--emsdk', type=existing_directory,
                        default=os.environ.get('EMSDK'),
                        help='emsdk to compile with. Its environment is derived here, so it '
                             'needs no activating beforehand; it does need to have been '
                             'installed ("emsdk install" and "emsdk activate"). Defaults to '
                             '$EMSDK, which an activated emsdk sets.')

    parser.add_argument('--src', type=existing_directory, default=source_root(),
                        help='Qt Creator sources to build (default: the checkout this '
                             'script belongs to)')
    parser.add_argument('--build', type=Path,
                        help="build directory (default: the preset's, "
                             '<src>/builds/wasm-qtprofiler)')
    parser.add_argument('--output', type=Path,
                        help='directory to copy the deployable files into, for a CI job to '
                             'archive. Replaced if it exists.')
    parser.add_argument('--zip', type=Path,
                        help='7z archive to pack the deployable files into, for a CI job to '
                             'upload. Replaced if it exists.')

    parser.add_argument('--build-type', default='Release',
                        help='CMake build type (default: %(default)s)')
    parser.add_argument('--jobs', '-j', type=int,
                        help='parallel compile jobs (default: as many as the generator picks)')
    parser.add_argument('--clean', action='store_true',
                        help='remove the build directory first, for a build from scratch. '
                             'With --build-qt, Qt is built from scratch as well.')

    qt = parser.add_argument_group(
        'building Qt',
        'Build the Qt for WebAssembly first, with the options in\n'
        'src/tools/qtprofiler/qt-configure-options.txt and the patches in\n'
        'src/tools/qtprofiler/qt-patches, and then qtprofiler against it.')
    qt.add_argument('--build-qt', action='store_true',
                    help='build Qt for WebAssembly before qtprofiler. It is built again only '
                         'when its options or its patches change, when it is built from other '
                         'sources or against another host Qt, or with --clean.')
    qt.add_argument('--qt-src', type=existing_directory,
                    help='Qt sources to build: a directory holding qtbase and qtsvg, such as '
                         'a qt5 checkout. The patches are applied to its qtbase, where it does '
                         f'not have them yet. Default: the Qt {QT_VERSION} source packages, '
                         'downloaded into the Qt build directory.')
    qt.add_argument('--qt-build', type=Path,
                    help='directory to build Qt in (default: <src>/builds/wasm-qt)')

    args = parser.parse_args()

    if not args.qt_build:
        args.qt_build = args.src / 'builds' / 'wasm-qt'
    args.qt_build = args.qt_build.expanduser().resolve()
    # A Qt installation handed over through the environment is one to build
    # against, never one to install over.
    if not args.qt_wasm:
        if args.build_qt:
            args.qt_wasm = args.qt_build / 'install'
        elif os.environ.get('QT_WASM_ROOT'):
            args.qt_wasm = Path(os.environ['QT_WASM_ROOT'])
    if args.qt_wasm:
        args.qt_wasm = args.qt_wasm.expanduser().resolve()

    missing = [name for name, value in [('--qt-wasm', args.qt_wasm), ('--qt-host', args.qt_host)]
               if value is None]
    if missing:
        parser.error('missing required argument(s): ' + ', '.join(missing))

    if not args.build:
        args.build = args.src / 'builds' / 'wasm-qtprofiler'
    args.build = args.build.expanduser().resolve()
    if args.output:
        args.output = args.output.expanduser().resolve()
    if args.zip:
        args.zip = args.zip.expanduser().resolve()
    return args


def emsdk_environment(emsdk: Path, env: dict[str, str]) -> dict[str, str]:
    """The environment an activated emsdk provides, without activating one.

    "emsdk construct_env" is what the emsdk_env scripts run to produce it, and
    it prints the result as shell assignments. Reading those is what lets this
    script be called directly rather than from a shell that sourced anything.
    """
    launcher = emsdk / ('emsdk.bat' if common.is_windows_platform() else 'emsdk')
    if not launcher.exists():
        sys.exit(f'{emsdk} does not look like an emsdk: no {launcher.name} in it.')

    result = subprocess.run([str(launcher), 'construct_env'],
                            cwd=str(emsdk), capture_output=True, text=True,
                            env={**env, 'EMSDK_QUIET': '1'})
    if result.returncode != 0:
        sys.exit(f'"{launcher} construct_env" failed:\n{result.stderr.strip()}\n'
                 'Has the emsdk been installed and activated once '
                 '("./emsdk install latest && ./emsdk activate latest")?')

    # POSIX emsdk prints 'export KEY="VALUE";'; the Windows one writes a batch
    # file of 'SET KEY=VALUE' instead, so accept both forms.
    assignments = dict(re.findall(r'^(?:export |SET )(\w+)=[\'"]?(.*?)[\'"]?;?$',
                                  result.stdout, re.MULTILINE))
    if not assignments:
        # Nothing to parse: fall back to what the env scripts set that actually
        # matters here, which is finding emcc and telling Qt where the emsdk is.
        assignments = {
            'EMSDK': str(emsdk),
            'PATH': os.pathsep.join([str(emsdk), str(emsdk / 'upstream' / 'emscripten'),
                                     env.get('PATH', '')]),
        }
    return {**env, **assignments}


def build_environment(args: argparse.Namespace) -> dict[str, str]:
    env = dict(os.environ)
    # What the preset reads for the toolchain file and the host tools.
    env['QT_WASM_ROOT'] = str(args.qt_wasm)
    env['QT_HOST_PATH'] = str(args.qt_host)
    if args.emsdk:
        env = emsdk_environment(args.emsdk, env)
    return env


def check_qt_wasm(args: argparse.Namespace) -> None:
    toolchain = args.qt_wasm / 'lib' / 'cmake' / 'Qt6' / 'qt.toolchain.cmake'
    if not toolchain.exists():
        sys.exit(f'--qt-wasm: {args.qt_wasm} is not a Qt installation: {toolchain} is missing.')


def check_prerequisites(args: argparse.Namespace, env: dict[str, str]) -> None:
    """Fail on a missing prerequisite before CMake does, and say which one."""
    if not args.build_qt:
        check_qt_wasm(args)
    if not (args.qt_host / 'lib' / 'cmake' / 'Qt6').is_dir():
        sys.exit(f'--qt-host: {args.qt_host} is not a Qt installation: '
                 'it has no lib/cmake/Qt6.')
    if not (args.src / 'CMakePresets.json').exists():
        sys.exit(f'--src: {args.src} has no CMakePresets.json, so it is not a Qt Creator '
                 'checkout.')

    tools = [('cmake', 'install CMake'),
             ('ninja', "the preset's generator; install Ninja"),
             ('emcc', 'pass --emsdk, or activate an emsdk')]
    if args.build_qt:
        tools.append(('git', '--build-qt applies the Qt patches with it; install git'))
        if not args.qt_src:
            tools.append(('tar', '--build-qt unpacks the Qt sources with it'))
    for tool, hint in tools:
        if not shutil.which(tool, path=env.get('PATH')):
            sys.exit(f'{tool} was not found in PATH ({hint}).')
    if args.zip and not (shutil.which('7zz') or shutil.which('7z')):
        sys.exit('--zip: neither 7zz nor 7z was found in PATH (install 7-Zip).')


def wasm_font_source(qt_wasm: Path) -> Path | None:
    """The directory holding the fonts Qt's platform plugin bundles, if it exists here.

    qtprofiler links a subset of them in place of Qt's copy; see
    src/tools/qtprofiler/CMakeLists.txt. Only Qt's sources have them, and a Qt
    install records where it was built from. A prebuilt Qt names a directory on
    the machine that built it, so the fonts are only found for a Qt built here.
    """
    extra = qt_wasm / 'lib' / 'cmake' / 'Qt6BuildInternals' / 'QtBuildInternalsExtra.cmake'
    if not extra.exists():
        return None
    match = re.search(r'set\(QT_SOURCE_TREE "([^"]+)"',
                      extra.read_text(encoding='utf-8', errors='replace'))
    if not match:
        return None
    fonts = Path(match.group(1)) / 'src' / '3rdparty' / 'wasm'
    if not all((fonts / name).exists() for name in ('DejaVuSans.ttf', 'DejaVuSansMono.ttf')):
        return None
    return fonts


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda: file.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def qt_sources(args: argparse.Namespace) -> Path:
    """The Qt sources to build: --qt-src, or the release packages, downloaded once."""
    if args.qt_src:
        return args.qt_src
    sources = args.qt_build / 'sources'
    series = QT_VERSION.rsplit('.', 1)[0]
    base_url = f'https://download.qt.io/official_releases/qt/{series}/{QT_VERSION}/submodules'
    for module, checksum in QT_SOURCE_SHA256.items():
        if (sources / module / 'CMakeLists.txt').exists():
            continue
        name = f'{module}-everywhere-src-{QT_VERSION}'
        archive = args.qt_build / 'downloads' / f'{name}.tar.xz'
        if not archive.exists():
            archive.parent.mkdir(parents=True, exist_ok=True)
            asyncio.run(common.download(f'{base_url}/{archive.name}', archive))
        if sha256(archive) != checksum:
            archive.unlink()
            sys.exit(f'{archive.name} does not have the checksum it should, so it was removed. '
                     'Run again to download it anew.')
        # Unpacked under the package's own name and renamed when complete, so an
        # interrupted run leaves nothing that looks finished.
        shutil.rmtree(sources / name, ignore_errors=True)
        common.extract_file(archive, sources)
        (sources / name).rename(sources / module)
    return sources


def qt_version_of_install(qt: Path) -> str | None:
    config = qt / 'lib' / 'cmake' / 'Qt6' / 'Qt6ConfigVersionImpl.cmake'
    if not config.exists():
        return None
    match = re.search(r'set\(PACKAGE_VERSION "([^"]+)"\)', config.read_text(encoding='utf-8'))
    return match.group(1) if match else None


def qt_version_of_sources(qtbase: Path) -> str | None:
    config = qtbase / '.cmake.conf'
    if not config.exists():
        return None
    match = re.search(r'set\(QT_REPO_MODULE_VERSION "([^"]+)"\)',
                      config.read_text(encoding='utf-8'))
    return match.group(1) if match else None


def qt_configure_options(args: argparse.Namespace) -> list[str]:
    options = qtprofiler_sources(args.src) / 'qt-configure-options.txt'
    return [option for line in options.read_text(encoding='utf-8').splitlines()
            if not line.lstrip().startswith('#') for option in line.split()]


def apply_qt_patches(args: argparse.Namespace, qtbase: Path) -> list[Path]:
    """Apply the patches Qt needs to qtbase, but for those it already has."""
    patches = sorted((qtprofiler_sources(args.src) / 'qt-patches' / 'qtbase').glob('*.patch'))
    # Unpacked sources are no repository of their own. Keep git from taking them
    # for part of the one they happen to be in, the Qt Creator checkout for the
    # default --qt-build, which need not even be readable: in a container, a
    # worktree's .git names a directory on the host. A qtbase that is a
    # repository, as in a qt5 checkout, is still found.
    env = {**os.environ, 'GIT_CEILING_DIRECTORIES': str(qtbase.parent)}
    for patch in patches:
        check = subprocess.run(['git', 'apply', '--reverse', '--check', str(patch)],
                               cwd=qtbase, env=env, capture_output=True, text=True)
        # 1 is "does not apply in reverse", so not in yet; anything else is git
        # failing, which must not pass for that.
        if check.returncode not in (0, 1):
            sys.exit(f'git could not check {patch.name} against {qtbase}:\n'
                     f'{check.stderr.strip()}')
        if check.returncode == 0:
            print(f'Already in the sources: {patch.name}')
        else:
            common.check_print_call(['git', 'apply', str(patch)], cwd=qtbase, env=env)
    return patches


def build_qt_module(args: argparse.Namespace, env: dict[str, str],
                    configure: list[str], build_dir: Path) -> None:
    build_dir.mkdir(parents=True)
    common.check_print_call(configure, cwd=build_dir, env=env)
    command = ['cmake', '--build', '.', '--parallel']
    if args.jobs:
        command.append(str(args.jobs))
    common.check_print_call(command, cwd=build_dir, env=env)
    common.check_print_call(['cmake', '--install', '.'], cwd=build_dir, env=env)


def build_qt(args: argparse.Namespace, env: dict[str, str]) -> None:
    """Build and install Qt for WebAssembly into --qt-wasm, unless it is there already.

    qtbase and qtsvg are built one after the other, as the Dockerfile does, so
    that they are all the sources needed.
    """
    sources = qt_sources(args)
    qtbase, qtsvg = sources / 'qtbase', sources / 'qtsvg'
    for module in (qtbase, qtsvg):
        if not (module / 'CMakeLists.txt').exists():
            sys.exit(f'--qt-src: {sources} has no {module.name} to build.')

    version = qt_version_of_sources(qtbase)
    host_version = qt_version_of_install(args.qt_host)
    if version != host_version:
        sys.exit(f'--qt-host: the host Qt has to be of the version that is built, {version}, '
                 f'but {args.qt_host} is {host_version or "of no version found"}.')

    patches = apply_qt_patches(args, qtbase)
    description = {
        'sources': str(sources),
        'host': str(args.qt_host),
        'build': str(args.qt_build),
        'options': qt_configure_options(args),
        'patches': {patch.name: sha256(patch) for patch in patches},
    }
    stamp = args.qt_wasm / QT_STAMP
    built = json.loads(stamp.read_text(encoding='utf-8')) if stamp.exists() else None
    if built is not None and not args.clean and built == {**description, 'complete': True}:
        print(f'Qt for WebAssembly in {args.qt_wasm} is up to date.')
        return
    if built is None and args.qt_wasm.exists() and any(args.qt_wasm.iterdir()):
        sys.exit(f'--qt-wasm: {args.qt_wasm} holds something this script did not install, '
                 'so it does not install Qt over it.')

    # Qt is configured from scratch: options can be turned off again, and an
    # install leaves the files of what it no longer builds behind.
    for directory in (args.qt_wasm, args.qt_build / 'qtbase', args.qt_build / 'qtsvg'):
        if directory.exists():
            print(f'Removing {directory}')
            shutil.rmtree(directory)
    args.qt_wasm.mkdir(parents=True)
    stamp.write_text(json.dumps({**description, 'complete': False}, indent=1) + '\n',
                     encoding='utf-8')

    windows = common.is_windows_platform()
    configure = qtbase / ('configure.bat' if windows else 'configure')
    build_qt_module(args, env, [str(configure), '-platform', 'wasm-emscripten',
                                '-prefix', str(args.qt_wasm),
                                '-qt-host-path', str(args.qt_host),
                                *description['options']],
                    args.qt_build / 'qtbase')
    configure_module = args.qt_wasm / 'bin' / ('qt-configure-module.bat' if windows
                                                else 'qt-configure-module')
    build_qt_module(args, env, [str(configure_module), str(qtsvg)], args.qt_build / 'qtsvg')

    stamp.write_text(json.dumps({**description, 'complete': True}, indent=1) + '\n',
                     encoding='utf-8')


def configure(args: argparse.Namespace, env: dict[str, str]) -> None:
    command = ['cmake', '--preset', PRESET, '-B', str(args.build),
               f'-DCMAKE_BUILD_TYPE={args.build_type}']
    fonts = wasm_font_source(args.qt_wasm)
    if fonts:
        command.append(f'-DQTPROFILER_WASM_FONT_SOURCE={fonts}')
    else:
        print(f"No sources found for {args.qt_wasm}: qtprofiler keeps Qt's full fonts.")
    # Run from the sources: that is where CMake looks for CMakePresets.json.
    common.check_print_call(command, cwd=args.src, env=env)


def build(args: argparse.Namespace, env: dict[str, str]) -> None:
    command = ['cmake', '--build', str(args.build), '--target', TARGET]
    if args.jobs:
        command += ['--parallel', str(args.jobs)]
    common.check_print_call(command, cwd=args.src, env=env)


def collect(args: argparse.Namespace) -> Path:
    """Check the build produced a servable application, and copy it if asked."""
    # add_qtc_executable() puts qtprofiler in a flat directory of its own rather
    # than in Qt Creator's libexec layout; see src/tools/qtprofiler/CMakeLists.txt.
    artifacts = args.build / 'qtprofiler'
    missing = [name for name in REQUIRED_ARTIFACTS if not (artifacts / name).exists()]
    if missing:
        sys.exit(f'The build did not produce {", ".join(missing)} in {artifacts}.')

    if args.output:
        if args.output.exists():
            shutil.rmtree(args.output)
        shutil.copytree(artifacts, args.output)
        artifacts = args.output

    print('------------------------------------------')
    print(f'qtprofiler for WebAssembly in {artifacts}:')
    for path in sorted(artifacts.iterdir()):
        if path.is_file():
            print(f'  {path.name:<24} {path.stat().st_size / 1024:>10.1f} KiB')
    print('Serve the directory over HTTP and open qtprofiler.html.')
    return artifacts


def pack(args: argparse.Namespace, artifacts: Path) -> None:
    """Pack the deployable files into a 7z archive, for a CI job to upload."""
    args.zip.parent.mkdir(parents=True, exist_ok=True)
    # "7z a" updates an existing archive instead of replacing it
    args.zip.unlink(missing_ok=True)
    common.check_print_call(common.sevenzip_command() + [str(args.zip), '*'],
                            cwd=artifacts)
    print(f'Packed into {args.zip}.')


def main() -> None:
    args = get_arguments()
    env = build_environment(args)
    check_prerequisites(args, env)

    if args.clean and args.build.exists():
        print(f'Removing {args.build}')
        shutil.rmtree(args.build)

    try:
        if args.build_qt:
            build_qt(args, env)
            check_qt_wasm(args)
        configure(args, env)
        build(args, env)
        artifacts = collect(args)
        if args.zip:
            pack(args, artifacts)
    except subprocess.CalledProcessError as error:
        # The command printed its own diagnostics above; a traceback on top of a
        # compiler error only buries it.
        sys.exit(f'{error.cmd[0]} failed with exit status {error.returncode}.')


if __name__ == '__main__':
    main()
