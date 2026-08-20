#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Build the standalone qtprofiler for Qt for WebAssembly.

Drives the qtprofiler-wasm CMake preset, which cross-compiles the trace viewer
against a Qt for WebAssembly installation. Everything the preset needs is taken
from the command line, so a CI job needs no prepared environment: point it at an
emsdk, a Qt for WebAssembly and the matching host Qt and it configures, builds
and collects the deployable files.

The cache variables stay in CMakePresets.json rather than being repeated here,
so the preset remains the one description of what this build is.
"""

from __future__ import annotations

import argparse
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


def source_root() -> Path:
    return Path(__file__).resolve().parent.parent


def existing_directory(value: str) -> Path:
    path = Path(value).expanduser()
    if not path.is_dir():
        raise argparse.ArgumentTypeError(f'no such directory: {path}')
    return path.resolve()


def get_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Build the standalone qtprofiler for Qt for WebAssembly.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
example:
  %(prog)s --emsdk ~/emsdk \\
      --qt-wasm ~/Qt/6.11.1/wasm_multithread \\
      --qt-host ~/Qt/6.11.1/macos \\
      --output artifacts --zip qtprofiler-wasm.7z

Each path also has an environment variable it falls back to, so an already
activated emsdk and a preset environment work without arguments.
'''.strip())

    parser.add_argument('--qt-wasm', type=existing_directory,
                        default=os.environ.get('QT_WASM_ROOT'),
                        help='Qt for WebAssembly installation to build against, '
                             'e.g. ~/Qt/6.11.1/wasm_multithread. The preset expects a '
                             'multithreaded build. Defaults to $QT_WASM_ROOT.')
    parser.add_argument('--qt-host', type=existing_directory,
                        default=os.environ.get('QT_HOST_PATH'),
                        help='Host Qt of the same version, whose tools run during the '
                             'cross build, e.g. ~/Qt/6.11.1/macos. Defaults to $QT_HOST_PATH.')
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
                        help='remove the build directory first, for a build from scratch')

    args = parser.parse_args()

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


def check_prerequisites(args: argparse.Namespace, env: dict[str, str]) -> None:
    """Fail on a missing prerequisite before CMake does, and say which one."""
    toolchain = args.qt_wasm / 'lib' / 'cmake' / 'Qt6' / 'qt.toolchain.cmake'
    if not toolchain.exists():
        sys.exit(f'--qt-wasm: {args.qt_wasm} is not a Qt installation: {toolchain} is missing.')
    if not (args.qt_host / 'lib' / 'cmake' / 'Qt6').is_dir():
        sys.exit(f'--qt-host: {args.qt_host} is not a Qt installation: '
                 'it has no lib/cmake/Qt6.')
    if not (args.src / 'CMakePresets.json').exists():
        sys.exit(f'--src: {args.src} has no CMakePresets.json, so it is not a Qt Creator '
                 'checkout.')

    for tool, hint in [('cmake', 'install CMake'),
                       ('ninja', "the preset's generator; install Ninja"),
                       ('emcc', 'pass --emsdk, or activate an emsdk')]:
        if not shutil.which(tool, path=env.get('PATH')):
            sys.exit(f'{tool} was not found in PATH ({hint}).')
    if args.zip and not (shutil.which('7zz') or shutil.which('7z')):
        sys.exit('--zip: neither 7zz nor 7z was found in PATH (install 7-Zip).')


def configure(args: argparse.Namespace, env: dict[str, str]) -> None:
    command = ['cmake', '--preset', PRESET, '-B', str(args.build),
               f'-DCMAKE_BUILD_TYPE={args.build_type}']
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
