import os
import sys

# These scripts need to continue using Python 2 rather than 3, because
# make_msvc_package.py puts the current Python interpreter on the PATH for the
# sake of gyp, and gyp doesn't work with Python 3 yet.
# https://bugs.chromium.org/p/gyp/issues/detail?id=36
if os.name != "nt":
    sys.exit("Error: ship scripts require native Python 2.7. (wrong os.name)")
if sys.version_info[0:2] != (2,7):
    sys.exit("Error: ship scripts require native Python 2.7. (wrong version)")

import glob
import hashlib
import shutil
import subprocess
from distutils.spawn import find_executable

topDir = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

with open(topDir + "/VERSION.txt", "rt") as f:
    winptyVersion = f.read().strip()

def rmrf(patterns):
    for pattern in patterns:
        for path in glob.glob(pattern):
            if os.path.isdir(path) and not os.path.islink(path):
                print("+ rm -r " + path)
                sys.stdout.flush()
                shutil.rmtree(path)
            elif os.path.isfile(path):
                print("+ rm " + path)
                sys.stdout.flush()
                os.remove(path)

def mkdir(path):
    if not os.path.isdir(path):
        os.makedirs(path)

def requireExe(name, guesses):
    if find_executable(name) is None:
        for guess in guesses:
            if os.path.exists(guess):
                newDir = os.path.dirname(guess)
                print("Adding " + newDir + " to Path to provide " + name)
                os.environ["Path"] = newDir + ";" + os.environ["Path"]
    ret = find_executable(name)
    if ret is None:
        sys.exit("Error: required EXE is missing from Path: " + name)
    return ret

class ModifyEnv:
    def __init__(self, **kwargs):
        self._changes = dict(kwargs)
        self._original = dict()

    def __enter__(self):
        for var, val in self._changes.items():
            self._original[var] = os.environ[var]
            os.environ[var] = val

    def __exit__(self, type, value, traceback):
        for var, val in self._original.items():
            os.environ[var] = val

def sha256(path):
    with open(path, "rb") as fp:
        return hashlib.sha256(fp.read()).hexdigest()

def checkSha256(path, expected):
    actual = sha256(path)
    if actual != expected:
        sys.exit("error: sha256 hash mismatch on {}: expected {}, found {}".format(
            path, expected, actual))

requireExe("git.exe", [
    "C:\\Program Files\\Git\\cmd\\git.exe",
    "C:\\Program Files (x86)\\Git\\cmd\\git.exe"
])

commitHash = subprocess.check_output(["git.exe", "rev-parse", "HEAD"]).strip()
defaultPathEnviron = "C:\\Windows\\System32;C:\\Windows"

ZIP_TOOL = requireExe("7z.exe", [
    "C:\\Program Files\\7-Zip\\7z.exe",
    "C:\\Program Files (x86)\\7-Zip\\7z.exe",
])

requireExe("curl.exe", [])
