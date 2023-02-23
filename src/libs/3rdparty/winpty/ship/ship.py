#!python

# Copyright (c) 2015 Ryan Prichard
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

#
# Run with native CPython 2.7 on a 64-bit computer.
#

import common_ship

from optparse import OptionParser
import multiprocessing
import os
import shutil
import subprocess
import sys


def dllVersion(path):
    version = subprocess.check_output(
        ["powershell.exe",
        "[System.Diagnostics.FileVersionInfo]::GetVersionInfo(\"" + path + "\").FileVersion"])
    return version.strip()


os.chdir(common_ship.topDir)


# Determine other build parameters.
BUILD_KINDS = {
    "cygwin": {
        "path": ["bin"],
        "dll": "bin\\cygwin1.dll",
    },
    "msys2": {
        "path": ["opt\\bin", "usr\\bin"],
        "dll": "usr\\bin\\msys-2.0.dll",
    },
}


def buildTarget(kind, syspath, arch):

    binPaths = [os.path.join(syspath, p) for p in BUILD_KINDS[kind]["path"]]
    binPaths += common_ship.defaultPathEnviron.split(";")
    newPath = ";".join(binPaths)

    dllver = dllVersion(os.path.join(syspath, BUILD_KINDS[kind]["dll"]))
    packageName = "winpty-{}-{}-{}-{}".format(common_ship.winptyVersion, kind, dllver, arch)
    if os.path.exists("ship\\packages\\" + packageName):
        shutil.rmtree("ship\\packages\\" + packageName)

    print("+ Setting PATH to: {}".format(newPath))
    with common_ship.ModifyEnv(PATH=newPath):
        subprocess.check_call(["sh.exe", "configure"])
        subprocess.check_call(["make.exe", "clean"])
        makeBaseCmd = [
            "make.exe",
            "COMMIT_HASH=" + common_ship.commitHash,
            "PREFIX=ship/packages/" + packageName
        ]
        subprocess.check_call(makeBaseCmd + ["all", "tests", "-j%d" % multiprocessing.cpu_count()])
        subprocess.check_call(["build\\trivial_test.exe"])
        subprocess.check_call(makeBaseCmd + ["install"])
        subprocess.check_call(["tar.exe", "cvfz",
            packageName + ".tar.gz",
            packageName], cwd=os.path.join(os.getcwd(), "ship", "packages"))


def main():
    parser = OptionParser()
    parser.add_option("--kind", type="choice", choices=["cygwin", "msys2"])
    parser.add_option("--syspath")
    parser.add_option("--arch", type="choice", choices=["ia32", "x64"])
    (args, extra) = parser.parse_args()

    args.kind or parser.error("--kind must be specified")
    args.arch or parser.error("--arch must be specified")
    args.syspath or parser.error("--syspath must be specified")
    extra and parser.error("unexpected positional argument(s)")

    buildTarget(args.kind, args.syspath, args.arch)


if __name__ == "__main__":
    main()
