# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Measures what an unresolved cdb file-and-line breakpoint costs while an
# application loads its libraries. See README.md.

import argparse
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

FIXTURE = Path(__file__).resolve().parent / "fixture"

LIB_SOURCE = """\
#include <windows.h>

extern "C" __declspec(dllexport) int work(int x)
{
    int y = x + 1;
    return y * 2;
}
"""

# The line the breakpoints go on, taken from the source rather than counted by
# hand, so editing the template above cannot silently move them elsewhere. The
# libraries are built unoptimized to keep that line a line with code.
BREAK_LINE = LIB_SOURCE.splitlines().index("    int y = x + 1;") + 1

MAIN_SOURCE = """\
#include <windows.h>
#include <stdio.h>

typedef int (*Work)(int);

int main()
{
    int total = 0;
    for (int i = 0; i < %d; ++i) {
        char name[64];
        sprintf(name, "lib%%03d.dll", i);
        HMODULE module = LoadLibraryA(name);
        if (!module) {
            printf("cannot load %%s\\n", name);
            return 1;
        }
        Work work = (Work)GetProcAddress(module, "work");
        if (!work) {
            printf("no work in %%s\\n", name);
            return 1;
        }
        total += work(i);
    }
    printf("total %%d\\n", total);
    return 0;
}
"""

NINJA_HEADER = """\
cflags = /nologo /c /Zi /Od /EHsc /MD /D_CRT_SECURE_NO_WARNINGS

rule cc
  command = cl $cflags /Fo$out /Fd$out.pdb $in
  description = CC $out

rule dll
  command = link /nologo /DLL /DEBUG /OUT:$out /IMPLIB:$implib /PDB:$pdb $in kernel32.lib
  description = DLL $out

rule exe
  command = link /nologo /DEBUG /OUT:$out /PDB:$pdb $in kernel32.lib
  description = EXE $out

"""

# " 0 e Disable Clear  00007ff6`1000 ..." resolved, " 1 eu ... (lib001.cpp:5)"
# deferred. The status letters carry a "u" exactly while cdb has not bound the
# expression to an address.
BREAKPOINT_LINE = re.compile(r"^\s*(\d+)\s+([edu]+)\s")

HIT_MARKER = "CDBBP_HIT"


def generate(dlls):
    FIXTURE.mkdir(parents=True, exist_ok=True)
    for i in range(dlls):
        (FIXTURE / ("lib%03d.cpp" % i)).write_text(LIB_SOURCE)
    (FIXTURE / "main.cpp").write_text(MAIN_SOURCE % dlls)

    lines = [NINJA_HEADER]
    targets = []
    for i in range(dlls):
        name = "lib%03d" % i
        lines.append("build obj/%s.obj: cc %s.cpp\n" % (name, name))
        lines.append("build %s.dll: dll obj/%s.obj\n" % (name, name))
        lines.append("  implib = obj/%s.lib\n" % name)
        lines.append("  pdb = %s.pdb\n\n" % name)
        targets.append("%s.dll" % name)
    lines.append("build obj/main.obj: cc main.cpp\n")
    lines.append("build app.exe: exe obj/main.obj\n")
    lines.append("  pdb = app.pdb\n\n")
    targets.append("app.exe")
    lines.append("default %s\n" % " ".join(targets))
    (FIXTURE / "build.ninja").write_text("".join(lines))


def build():
    ninja = shutil.which("ninja")
    if not ninja:
        sys.exit("ninja is not on PATH")
    if not shutil.which("cl"):
        sys.exit("cl is not on PATH, run this from a Visual Studio command prompt")
    subprocess.run([ninja], cwd=FIXTURE, check=True)


def breakpoint_locations(dlls, count):
    # Spread them over the libraries, so the modules they name are loaded at
    # different points of the run rather than all at the start.
    indexes = sorted({min(dlls - 1, i * dlls // count) for i in range(count)})
    return [("lib%03d" % i, "lib%03d.cpp" % i) for i in indexes]


def parse_breakpoints(output):
    # Only the listing the script asked for at the end, not the commands cdb
    # echoed while setting the breakpoints.
    tail = output.rsplit("> bl", 1)[-1]
    total = resolved = 0
    for line in tail.splitlines():
        match = BREAKPOINT_LINE.match(line)
        if not match:
            continue
        total += 1
        if "u" not in match.group(2):
            resolved += 1
    return resolved, total


def measure(cdb, locations, scoped):
    commands = []
    for module, source in locations:
        where = "%s!%s:%d" % (module, source, BREAK_LINE) if scoped \
            else "%s:%d" % (source, BREAK_LINE)
        commands.append('bu `%s` ".echo %s;gc"' % (where, HIT_MARKER))
    # No -G: the break on process exit is what keeps cdb alive for the listing.
    commands += ["g", "bl", "q"]
    script = FIXTURE / "cdbscript.txt"
    script.write_text("\n".join(commands) + "\n")

    started = time.perf_counter()
    run = subprocess.run([cdb, "-cf", str(script), "app.exe"],
                         cwd=FIXTURE, capture_output=True, text=True)
    elapsed = time.perf_counter() - started
    output = run.stdout + run.stderr
    resolved, total = parse_breakpoints(output)
    return elapsed, resolved, total, output.count(HIT_MARKER)


def main():
    parser = argparse.ArgumentParser(
        description="Measure what an unresolved cdb file-and-line breakpoint costs.")
    parser.add_argument("--dlls", type=int, default=200,
                        help="number of libraries the generated application loads")
    parser.add_argument("--breakpoints", default="0,1,10,36",
                        help="comma separated breakpoint counts to measure")
    parser.add_argument("--keep", action="store_true",
                        help="reuse an already generated fixture")
    parser.add_argument("--cdb", default=shutil.which("cdb"),
                        help="the cdb.exe to run")
    args = parser.parse_args()

    if not args.cdb:
        sys.exit("no cdb.exe found on PATH, pass --cdb")
    counts = [int(c) for c in args.breakpoints.split(",")]
    if max(counts) > args.dlls:
        sys.exit("cannot set %d breakpoints in %d libraries" % (max(counts), args.dlls))

    if args.keep and (FIXTURE / "app.exe").exists():
        print("reusing %s" % FIXTURE)
    else:
        generate(args.dlls)
        build()

    print("%-12s %-7s %8s  %-9s %s" % ("breakpoints", "mode", "time", "resolved", "hit"))
    for count in counts:
        locations = breakpoint_locations(args.dlls, count) if count else []
        for scoped in [False, True]:
            if scoped and not locations:
                continue
            elapsed, resolved, total, hits = measure(args.cdb, locations, scoped)
            print("%-12d %-7s %7.2fs  %-9s %d"
                  % (count, "scoped" if scoped else "plain", elapsed,
                     "%d/%d" % (resolved, total), hits))


if __name__ == "__main__":
    main()
