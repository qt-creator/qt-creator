#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Lay out the toolchain that Qt Creator's package carries for building on the device.

Qt Creator running on a HarmonyOS device builds with what its package brings
in qtctools.hnp: clang, lld, cmake and ninja, the sysroot and the libc++ of
the device-hosted native SDK, bmake as the make that runs qmake's Makefiles,
and lldb with the Python it embeds, beside the lldb-server of the SDK given
as --sdk.

The device-hosted native SDK is the one published as "native-ohos-x64", whose
payload is for arm64 despite the name. It has no make and no lldb, so those
are cross-built here with the clang of the SDK given as --sdk:

- bmake, from its release tarball.
- CPython 3.10, which lldb embeds, cross-built against a host build of the
  same version. Its extension modules have to name libpython themselves,
  because the musl of OpenHarmony does not let a module that is loaded later
  reach the symbols of a libpython loaded as a dependency.
- lldb 15 from llvm-project, with the patch in patches/lldb-ohos.patch: a
  platform plugin that answers only when asked for by name, and a fix for the
  Python file handles on musl. Its table generators and SWIG 4.0.2 are built
  for the host first.

All sources are pinned downloads. The work trees stay in --work-path and a
later run reuses whatever is already built. The toolchain goes into --path,
which is replaced.

Building needs cmake, ninja, patchelf, patch, a host C and C++ compiler and
make.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tarfile
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent

DEVICE_PREFIX = "/data/app/qtctools.org/qtctools_1.0"
TRIPLE = "aarch64-linux-ohos"

SOURCES = {
    "bmake": ("https://www.crufty.net/ftp/pub/sjg/bmake-20260912.tar.gz",
              "b6bd32964cbe451be2838822c9d200b7c7e76a2a5947c03feb71dc6bd72988bd",
              "bmake"),
    "python": ("https://www.python.org/ftp/python/3.10.14/Python-3.10.14.tgz",
               "cefea32d3be89c02436711c95a45c7f8e880105514b78680c14fe76f5709a0f6",
               "Python-3.10.14"),
    "swig": ("https://downloads.sourceforge.net/project/swig/swig/swig-4.0.2/"
             "swig-4.0.2.tar.gz",
             "d53be9730d8d58a16bf0cbd1f8ac0c0c3e1090573168bfa151b01eb47fa906fc",
             "swig-4.0.2"),
    "llvm": ("https://github.com/llvm/llvm-project/archive/"
             "7283f48a05de46fe8721ee6c29b1b6427e7d1a33.tar.gz",
             "40502d31d75b40d856127ba063bff710b51a3599614dc8e7379b6a07e7428157",
             "llvm-project-7283f48a05de46fe8721ee6c29b1b6427e7d1a33"),
}

PYTHON = "python3.10"

# CPython's configure cannot run test programs for another host.
PYTHON_CONFIG_SITE = """\
ac_cv_file__dev_ptmx=no
ac_cv_file__dev_ptc=no
ac_cv_buggy_getaddrinfo=no
ac_cv_header_libintl_h=no
"""

# CPython's configure rejects a compiler whose --print-multiarch disagrees with the
# triplet it derives from the predefined macros, so that query answers empty.
CLANG_WRAPPER = """\
#!/bin/sh
for a in "$@"; do [ "$a" = "--print-multiarch" ] && exit 0; done
exec "{clang}" --target={triple} --sysroot="{sysroot}" "$@"
"""

# Not needed for running lldb's scripts.
PYTHON_EXCLUDED = {"test", "idlelib", "lib2to3", "tkinter", "turtledemo", "ensurepip",
                   "pydoc_data"}

LLDB_TARGET_ARGUMENTS = [
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_SYSTEM_NAME=Linux",
    "-DCMAKE_SYSTEM_PROCESSOR=aarch64",
    "-DCMAKE_CXX_FLAGS=-include cstdint",
    "-DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ -Wl,-rpath,{}/lib".format(DEVICE_PREFIX),
    "-DCMAKE_INSTALL_PREFIX={}".format(DEVICE_PREFIX),
    "-DLLVM_TARGETS_TO_BUILD=AArch64",
    "-DLLVM_DEFAULT_TARGET_TRIPLE={}".format(TRIPLE),
    "-DLLVM_HOST_TRIPLE=aarch64-unknown-linux-musl",
    "-DLLVM_ENABLE_PROJECTS=clang;lldb",
    "-DLLVM_ENABLE_ASSERTIONS=OFF",
    "-DLLVM_INCLUDE_TESTS=OFF",
    "-DLLVM_INCLUDE_BENCHMARKS=OFF",
    "-DLLVM_INCLUDE_EXAMPLES=OFF",
    "-DLLVM_ENABLE_TERMINFO=OFF",
    "-DLLVM_ENABLE_ZLIB=OFF",
    "-DLLVM_ENABLE_ZSTD=OFF",
    "-DLLVM_ENABLE_LIBXML2=OFF",
    "-DLLVM_ENABLE_LIBEDIT=OFF",
    "-DLLDB_ENABLE_CURSES=OFF",
    "-DLLDB_ENABLE_LIBEDIT=OFF",
    "-DLLDB_ENABLE_LZMA=OFF",
    "-DLLDB_ENABLE_LUA=OFF",
    "-DLLDB_ENABLE_LIBXML2=OFF",
    "-DLLDB_ENABLE_PYTHON=ON",
    "-DLLDB_PYTHON_RELATIVE_PATH=lib/{}/site-packages".format(PYTHON),
    "-DLLDB_PYTHON_EXE_RELATIVE_PATH=bin/{}".format(PYTHON),
    "-DLLDB_PYTHON_EXT_SUFFIX=.so",
]


def absolute(path: str) -> Path:
    return Path(path).expanduser().resolve()


def run(command: list, cwd: Path | None = None, env: dict | None = None,
        log: Path | None = None) -> None:
    command = [str(part) for part in command]
    print("+ {}".format(" ".join(command)), flush=True)
    environment = dict(os.environ)
    # A CCACHE_PREFIX of icecc makes every compiler call go through a launcher
    # that cannot reach the cross compiler.
    environment.pop("CCACHE_PREFIX", None)
    environment.update(env or {})
    if log:
        with open(log, "a") as output:
            result = subprocess.run(command, cwd=cwd, env=environment, stdout=output,
                                    stderr=subprocess.STDOUT)
    else:
        result = subprocess.run(command, cwd=cwd, env=environment)
    if result.returncode != 0:
        raise SystemExit("{} failed{}".format(Path(command[0]).name,
                                              ", see {}".format(log) if log else ""))


def openharmony_path(sdk: Path) -> Path:
    """The "openharmony" directory inside whatever the SDK argument points at."""
    for candidate in ["sdk/default/openharmony", "default/openharmony", "openharmony", ""]:
        root = sdk / candidate if candidate else sdk
        if (root / "native" / "sysroot").is_dir():
            return root
    raise SystemExit("No OpenHarmony SDK with a native/sysroot under {}".format(sdk))


def only_directory(pattern: str, parent: Path) -> Path:
    found = sorted(parent.glob(pattern))
    if len(found) != 1:
        raise SystemExit("Expected one {} in {}, found {}".format(pattern, parent, len(found)))
    return found[0]


def source(name: str, work: Path) -> Path:
    """Download and unpack a pinned source tarball, once."""
    url, sha256, directory = SOURCES[name]
    unpacked = work / "src" / directory
    if unpacked.is_dir():
        return unpacked
    archive = work / "downloads" / url.rsplit("/", 1)[1]
    if not archive.is_file():
        archive.parent.mkdir(parents=True, exist_ok=True)
        print("Downloading {}".format(url), flush=True)
        partial = archive.with_name(archive.name + ".part")
        urllib.request.urlretrieve(url, str(partial))
        partial.rename(archive)
    digest = hashlib.sha256()
    with open(archive, "rb") as data:
        for chunk in iter(lambda: data.read(1 << 20), b""):
            digest.update(chunk)
    if digest.hexdigest() != sha256:
        raise SystemExit("Checksum mismatch for {}".format(archive))
    staging = work / "src" / (directory + ".unpacking")
    shutil.rmtree(staging, ignore_errors=True)
    staging.mkdir(parents=True)
    with tarfile.open(archive) as tar:
        tar.extractall(staging, filter="tar")
    (staging / directory).rename(unpacked)
    staging.rmdir()
    return unpacked


def device_native(device_sdk: Path, work: Path) -> Path:
    """The "native" directory of the device-hosted SDK, unpacked from its zip if need be."""
    if device_sdk.is_dir():
        for candidate in [device_sdk / "native", device_sdk]:
            if (candidate / "llvm" / "bin" / "clang").is_file():
                return candidate
        raise SystemExit("No native SDK with llvm/bin/clang in {}".format(device_sdk))
    native = work / "device-sdk" / "native"
    if (native / "llvm" / "bin" / "clang").is_file():
        return native
    shutil.rmtree(native.parent, ignore_errors=True)
    print("Unpacking {}".format(device_sdk), flush=True)
    # zipfile drops the permission bits, unzip keeps them.
    run(["unzip", "-q", device_sdk, "-d", native.parent])
    if not (native / "llvm" / "bin" / "clang").is_file():
        raise SystemExit("{} has no native/llvm/bin/clang".format(device_sdk))
    return native


def clang_wrappers(work: Path, sdk_native: Path) -> tuple[Path, Path]:
    bin = work / "bin"
    bin.mkdir(parents=True, exist_ok=True)
    wrappers = []
    for name, driver in [("ohos-clang", "clang"), ("ohos-clang++", "clang++")]:
        wrapper = bin / name
        wrapper.write_text(CLANG_WRAPPER.format(clang=sdk_native / "llvm" / "bin" / driver,
                                                triple=TRIPLE,
                                                sysroot=sdk_native / "sysroot"))
        wrapper.chmod(0o755)
        wrappers.append(wrapper)
    return wrappers[0], wrappers[1]


def build_bmake(work: Path, cc: Path, jobs: str) -> Path:
    build = work / "build-bmake"
    bmake = build / "bmake"
    if bmake.is_file():
        return bmake
    shutil.rmtree(build, ignore_errors=True)
    build.mkdir(parents=True)
    log = work / "bmake.log"
    run([source("bmake", work) / "configure", "--host={}".format(TRIPLE),
         "--with-default-sys-path={}/share/mk".format(DEVICE_PREFIX)],
        cwd=build, env={"CC": str(cc)}, log=log)
    run(["sh", "./make-bootstrap.sh"], cwd=build, log=log)
    return bmake


def build_host_tools(work: Path, jobs: str) -> tuple[Path, Path]:
    """SWIG and the Python that the cross builds of CPython and lldb run on the host."""
    host = work / "host"
    swig = host / "bin" / "swig"
    python = host / "bin" / PYTHON
    host_env = {"CC": "cc", "CXX": "c++"}
    if not swig.is_file():
        build = work / "build-host-swig"
        shutil.rmtree(build, ignore_errors=True)
        build.mkdir(parents=True)
        log = work / "host-swig.log"
        run([source("swig", work) / "configure", "--prefix={}".format(host), "--without-pcre"],
            cwd=build, env=host_env, log=log)
        run(["make", "-j", jobs], cwd=build, log=log)
        run(["make", "install"], cwd=build, log=log)
    if not python.is_file():
        build = work / "build-host-python"
        shutil.rmtree(build, ignore_errors=True)
        build.mkdir(parents=True)
        log = work / "host-python.log"
        run([source("python", work) / "configure", "--prefix={}".format(host)],
            cwd=build, env=host_env, log=log)
        run(["make", "-j", jobs], cwd=build, log=log)
        run(["make", "install"], cwd=build, log=log)
    return swig, python


def build_target_python(work: Path, cc: Path, cxx: Path, sdk_native: Path, host_python: Path,
                        jobs: str) -> Path:
    root = work / "target-python-root"
    prefix = root / DEVICE_PREFIX.lstrip("/")
    if (prefix / "lib" / "libpython3.10.so.1.0").is_file():
        return prefix
    build = work / "build-target-python"
    shutil.rmtree(build, ignore_errors=True)
    shutil.rmtree(root, ignore_errors=True)
    build.mkdir(parents=True)
    (build / "config.site").write_text(PYTHON_CONFIG_SITE)
    llvm_bin = sdk_native / "llvm" / "bin"
    env = {"CC": str(cc), "CXX": str(cxx), "AR": str(llvm_bin / "llvm-ar"),
           "RANLIB": str(llvm_bin / "llvm-ranlib"), "READELF": str(llvm_bin / "llvm-readelf"),
           "CFLAGS": "-O2 -fPIC", "LDFLAGS": "-Wl,-rpath,{}/lib".format(DEVICE_PREFIX),
           "CONFIG_SITE": str(build / "config.site"),
           "PATH": "{}:{}".format(host_python.parent, os.environ["PATH"])}
    log = work / "target-python.log"
    run([source("python", work) / "configure", "--host=aarch64-linux-musl",
         "--build=x86_64-pc-linux-gnu", "--prefix={}".format(DEVICE_PREFIX), "--enable-shared",
         "--without-ensurepip", "--with-build-python={}".format(host_python)],
        cwd=build, env=env, log=log)
    run(["make", "-j", jobs], cwd=build, env=env, log=log)
    # The extension modules are linked again, now against libpython.
    for module in (build / "build").rglob("*.so"):
        module.unlink()
    shared = "BLDSHARED={} -shared -L{} -lpython3.10 -Wl,-rpath,{}/lib".format(
        cc, build, DEVICE_PREFIX)
    run(["make", "-j", jobs, shared, "sharedmods"], cwd=build, env=env, log=log)
    run(["make", "install", "DESTDIR={}".format(root), shared], cwd=build, env=env, log=log)
    return prefix


def llvm_source(work: Path) -> Path:
    patched = work / "src" / "llvm-project-ohos"
    if patched.is_dir():
        return patched
    unpacked = source("llvm", work)
    run(["patch", "-p1", "-i", HERE / "patches" / "lldb-ohos.patch"], cwd=unpacked)
    unpacked.rename(patched)
    return patched


def build_tablegen(work: Path, llvm: Path) -> Path:
    build = work / "build-host-llvm"
    if not (build / "bin" / "lldb-tblgen").is_file():
        log = work / "host-llvm.log"
        run(["cmake", "-S", llvm / "llvm", "-B", build, "-GNinja", "-DCMAKE_BUILD_TYPE=Release",
             "-DCMAKE_C_COMPILER=cc", "-DCMAKE_CXX_COMPILER=c++",
             "-DCMAKE_CXX_FLAGS=-include cstdint", "-DLLVM_ENABLE_PROJECTS=clang;lldb",
             "-DLLVM_TARGETS_TO_BUILD=AArch64", "-DLLVM_ENABLE_ASSERTIONS=OFF",
             "-DLLDB_ENABLE_PYTHON=OFF", "-DLLDB_ENABLE_CURSES=OFF",
             "-DLLDB_ENABLE_LIBEDIT=OFF", "-DLLVM_INCLUDE_TESTS=OFF",
             "-DLLVM_INCLUDE_BENCHMARKS=OFF"], log=log)
        run(["ninja", "-C", build, "llvm-tblgen", "clang-tblgen", "lldb-tblgen", "llvm-config"],
            log=log)
    return build / "bin"


def build_lldb(work: Path, llvm: Path, cc: Path, cxx: Path, tablegen: Path, swig: Path,
               host_python: Path, python_prefix: Path) -> Path:
    build = work / "build-target-llvm"
    if (build / "bin" / "lldb").is_file():
        return build
    log = work / "target-lldb.log"
    run(["cmake", "-S", llvm / "llvm", "-B", build, "-GNinja",
         "-DCMAKE_C_COMPILER={}".format(cc), "-DCMAKE_CXX_COMPILER={}".format(cxx),
         "-DLLVM_TABLEGEN={}".format(tablegen / "llvm-tblgen"),
         "-DCLANG_TABLEGEN={}".format(tablegen / "clang-tblgen"),
         "-DLLDB_TABLEGEN={}".format(tablegen / "lldb-tblgen"),
         "-DLLVM_CONFIG_PATH={}".format(tablegen / "llvm-config"),
         "-DSWIG_EXECUTABLE={}".format(swig),
         "-DPython3_EXECUTABLE={}".format(host_python),
         "-DPython3_INCLUDE_DIR={}".format(python_prefix / "include" / PYTHON),
         "-DPython3_LIBRARY={}".format(python_prefix / "lib" / "libpython3.10.so")]
        + LLDB_TARGET_ARGUMENTS, log=log)
    run(["ninja", "-C", build, "lldb"], log=log)
    return build


def copy_tree(source_dir: Path, target: Path, ignore=None) -> None:
    shutil.copytree(source_dir, target, symlinks=True, ignore=ignore, dirs_exist_ok=True)


def lay_out(out: Path, native: Path, llvm_bin: Path, bmake: Path, lldb_build: Path,
            python_prefix: Path) -> None:
    version = only_directory("llvm/lib/clang/*", native).name
    shutil.rmtree(out, ignore_errors=True)
    for directory in ["bin", "include", "lib/clang/{}".format(version), "share", "build/cmake"]:
        (out / directory).mkdir(parents=True, exist_ok=True)
    bin = out / "bin"

    # The driver picks its language from the name it is called by.
    shutil.copy2(native / "llvm/bin/clang", bin / "clang")
    shutil.copy2(native / "llvm/bin/clang", bin / "clang++")
    for tool in ["ld.lld", "llvm-ar", "llvm-ranlib", "llvm-strip"]:
        shutil.copy2(native / "llvm/bin" / tool, bin / tool)
    for tool in ["cmake", "ninja"]:
        shutil.copy2(native / "build-tools/cmake/bin" / tool, bin / tool)

    # ld.lld needs libxml2.
    readelf = subprocess.run([str(llvm_bin / "llvm-readelf"), "-d", str(bin / "ld.lld")],
                             capture_output=True, text=True, check=True).stdout
    for name in re.findall(r"\(NEEDED\).*\[(libxml2[^]]*)\]", readelf):
        shutil.copy2(native / "llvm/lib" / name, out / "lib")
    # cmake and ninja have no runpath, so they would get the system's libc++.
    shutil.copy2(native / "llvm/lib" / TRIPLE / "libc++_shared.so", out / "lib")
    run(["patchelf", "--set-rpath", "$ORIGIN/../lib", bin / "cmake", bin / "ninja"])

    copy_tree(native / "llvm/include/libcxx-ohos", out / "include/libcxx-ohos")
    resource = out / "lib/clang" / version
    copy_tree(native / "llvm/lib/clang" / version / "include", resource / "include")
    runtime = native / "llvm/lib/clang" / version / "lib" / TRIPLE
    (resource / "lib" / TRIPLE).mkdir(parents=True)
    # Not the sanitizers, they are 40 MB.
    for name in ["libclang_rt.builtins.a", "clang_rt.crtbegin.o", "clang_rt.crtend.o"]:
        shutil.copy2(runtime / name, resource / "lib" / TRIPLE / name)
    copy_tree(native / "llvm/lib" / TRIPLE, out / "lib" / TRIPLE)
    for share in native.glob("build-tools/cmake/share/cmake-*"):
        copy_tree(share, out / "share" / share.name)
    copy_tree(native / "sysroot", out / "sysroot")
    shutil.copy2(native / "oh-uni-package.json", out)

    # The SDK's toolchain file assumes an llvm subdirectory, the package has a flat bin.
    toolchain = (native / "build/cmake/ohos.toolchain.cmake").read_text()
    toolchain = toolchain.replace('"${OHOS_SDK_NATIVE}/llvm/bin"', '"${OHOS_SDK_NATIVE}/bin"')
    toolchain = toolchain.replace('"${OHOS_SDK_NATIVE}/llvm"', '"${OHOS_SDK_NATIVE}"')
    (out / "build/cmake/ohos.toolchain.cmake").write_text(toolchain)
    shutil.copy2(native / "build/cmake/sdk_native_platforms.cmake", out / "build/cmake")

    strip = llvm_bin / "llvm-strip"
    run([strip, "--strip-all", "-o", bin / "make", bmake])
    # bmake refuses to run without a sys.mk. qmake's Makefiles need nothing from it.
    (out / "share/mk").mkdir(parents=True)
    (out / "share/mk/sys.mk").write_text("")

    run([strip, "--strip-all", "-o", bin / "lldb", lldb_build / "bin/lldb"])
    # The debugged program runs under the server of the host SDK, which is static.
    server = only_directory("lib/clang/*", llvm_bin.parent) / "bin" / TRIPLE / "lldb-server"
    shutil.copy2(server, bin / "lldb-server")
    run([strip, "--strip-all", "-o", out / "lib/liblldb.so.15git",
         lldb_build / "lib/liblldb.so.15git"])
    python_lib = python_prefix / "lib"
    shutil.copy2(python_lib / "libpython3.10.so.1.0", out / "lib")

    def ignore_python(directory: str, names: list[str]) -> set[str]:
        if Path(directory) == python_lib / PYTHON:
            return {name for name in names
                    if name in PYTHON_EXCLUDED or name.startswith("config-3.10-")}
        if Path(directory).parent == python_lib / PYTHON:
            return {"test"} & set(names)
        return set()

    copy_tree(python_lib / PYTHON, out / "lib" / PYTHON, ignore=ignore_python)
    # The _lldb.so and lldb-argdumper links would dangle.
    copy_tree(lldb_build / "lib" / PYTHON / "site-packages", out / "lib" / PYTHON / "site-packages",
              ignore=lambda directory, names: {"_lldb.so", "lldb-argdumper"} & set(names))

    for directory in [bin, out / "lib"]:
        for entry in directory.iterdir():
            if not entry.is_symlink():
                entry.chmod(0o755)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sdk", type=absolute, required=True,
                        help="HarmonyOS SDK for the host, whose clang cross-builds bmake, "
                             "Python and lldb")
    parser.add_argument("--device-sdk", type=absolute, required=True,
                        help="the device-hosted native SDK, either its native-ohos-x64 zip or "
                             "the directory it was unpacked into")
    parser.add_argument("--path", type=absolute, required=True,
                        help="directory to lay the toolchain out in, replaced")
    parser.add_argument("--work-path", type=absolute,
                        help="where to keep the sources and the build trees, \"<path>-work\" "
                             "by default. Reused by a later run")
    parser.add_argument("--jobs", type=int, default=os.cpu_count(),
                        help="parallel build jobs")
    args = parser.parse_args()

    for tool in ["cmake", "ninja", "patchelf", "patch", "make", "cc", "c++"]:
        if not shutil.which(tool):
            raise SystemExit("{} is missing on this host".format(tool))

    work = args.work_path or args.path.with_name(args.path.name + "-work")
    work.mkdir(parents=True, exist_ok=True)
    jobs = str(args.jobs)
    sdk_native = openharmony_path(args.sdk) / "native"
    native = device_native(args.device_sdk, work)
    cc, cxx = clang_wrappers(work, sdk_native)

    bmake = build_bmake(work, cc, jobs)
    swig, host_python = build_host_tools(work, jobs)
    python_prefix = build_target_python(work, cc, cxx, sdk_native, host_python, jobs)
    llvm = llvm_source(work)
    tablegen = build_tablegen(work, llvm)
    lldb_build = build_lldb(work, llvm, cc, cxx, tablegen, swig, host_python, python_prefix)

    lay_out(args.path, native, sdk_native / "llvm" / "bin", bmake, lldb_build, python_prefix)
    print("\nLaid out the device toolchain in {}".format(args.path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
