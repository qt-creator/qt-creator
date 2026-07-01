# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Standalone checks for the value dumper in pdbbridge.py. Invoked by tst_pdb.
# The location of pdbbridge.py is passed via the DUMPERDIR environment variable
# (or as the first argument). Exits with a non-zero status if a check fails.
#
# The PySide6-specific checks are skipped when PySide6 is not installed, so the
# basic checks still run on a plain Python installation.

import binascii
import os
import re
import sys

dumperDir = os.environ.get("DUMPERDIR") or (sys.argv[1] if len(sys.argv) > 1 else "")
source = open(os.path.join(dumperDir, "pdbbridge.py")).read()
# Drop the module-level bootstrap that would start the interactive pdb loop.
source = re.sub(r"(?m)^__the_dumper__.*$", "", source)
glob = {"__builtins__": __import__("builtins"), "__name__": "pdbbridge_under_test"}
exec(compile(source, "pdbbridge.py", "exec"), glob)
Dumper = glob["QtcInternalDumper"]

failures = []


def dump(value, expanded=True, iname="local.x"):
    d = Dumper()
    d.expandedINames = [iname] if expanded else []
    d.output = ""
    d.dumpValue(value, "x", iname)
    return d.output


def check(condition, message):
    if not condition:
        failures.append(message)


def decodeHex(text):
    return binascii.unhexlify(text).decode("utf8")


def encodedValue(output, pattern=r'value="([0-9A-Fa-f]*)",valueencoded="utf8"'):
    match = re.search(pattern, output)
    return decodeHex(match.group(1)) if match else None


# Basic Python types.
out = dump(42)
check('type="int"' in out and 'value="42"' in out, "int: %s" % out)

out = dump("hi")
check(encodedValue(out) == "hi", "str: %s" % out)

# Non-ASCII survives the round-trip.
out = dump("caf\u00e9")
check(encodedValue(out) == "caf\u00e9", "unicode str: %s" % out)

out = dump([10, 20])
check('type="list"' in out and 'value="<2 items>"' in out, "list count: %s" % out)
check('name="0"' in out and 'value="10"' in out, "list items: %s" % out)

out = dump({"k": 1})
check('type="dict"' in out, "dict: %s" % out)

out = dump(None)
check('type="NoneType"' in out and "None" in out, "none: %s" % out)

out = dump(True)
check('type="bool"' in out and 'value="True"' in out, "bool: %s" % out)


# Plain object: dir()-based children, with methods filtered out.
class Plain:
    def __init__(self):
        self.field = 7

    def method(self):
        return 0


out = dump(Plain())
check('name="field"' in out and 'value="7"' in out, "plain field: %s" % out)
check('name="method"' not in out, "plain method must be filtered: %s" % out)


# The object repr used as value is encoded, so quotes do not corrupt the data.
class Quoting:
    def __repr__(self):
        return 'a"b,c'


out = dump(Quoting(), expanded=False)
check('valueencoded="utf8"' in out, "repr must be encoded: %s" % out)
check(encodedValue(out) == 'a"b,c', "repr round-trip: %s" % out)

# The meta-property helper only reacts to PySide/Shiboken objects.
check(Dumper.qtMetaProperties(42) == [], "qtMetaProperties(int) must be empty")
check(Dumper.qtMetaProperties(Plain()) == [], "qtMetaProperties(plain) must be empty")


# PySide QObjects expose their state as Qt meta-properties.
try:
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    from PySide6.QtWidgets import QApplication, QLabel

    app = QApplication.instance() or QApplication([])
    label = QLabel("Hello")
    label.setObjectName("myLabel")

    props = Dumper.qtMetaProperties(label)
    check("text" in props and "objectName" in props,
          "QLabel meta-properties: %s" % props)

    out = dump(label)
    check('numchild="' in out, "QLabel must be expandable: %s" % out)
    text = encodedValue(out, r'name="text",type="str",value="([0-9A-Fa-f]*)"')
    check(text == "Hello", "QLabel text property: %s" % out)
    name = encodedValue(out, r'name="objectName",type="str",value="([0-9A-Fa-f]*)"')
    check(name == "myLabel", "QLabel objectName property: %s" % out)
    print("PySide6 checks: OK")
except ImportError:
    print("PySide6 not available, skipping QObject checks.")


if failures:
    print("FAILED %d check(s):" % len(failures))
    for failure in failures:
        print("  -", failure)
    sys.exit(1)

print("All pdb dumper checks passed.")
sys.exit(0)
