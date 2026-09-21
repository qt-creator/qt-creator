# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Checks how often the cdb bridge asks the debugger engine about types.

Every method of a cdbext.Value or cdbext.Type is a round trip into dbgeng, so
the bridge has to ask once and keep the answer. The fakes below count the
questions; the checks pin the counts a value and a type cost.

    python tst_cdbbridge.py
"""

import os
import sys
import types
from collections import Counter

DEBUGGER_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            '..', '..', '..', 'share', 'qtcreator', 'debugger')
sys.path.insert(0, os.path.normpath(DEBUGGER_DIR))

from utils import TypeCode

# cdbext is provided by the CDB extension and cannot be imported here.
_cdbext = types.ModuleType('cdbext')
_cdbext.Type = type('Type', (), {})
_cdbext.Value = type('Value', (), {})
_cdbext.lookupType = lambda name, module=0: None
_cdbext.listOfModules = lambda: []
_cdbext.pointerSize = lambda: 8
sys.modules['cdbext'] = _cdbext

import cdbbridge

failures = []
calls = Counter()


def expect(what, actual, expected):
    if actual == expected:
        print('  PASS  %s: %r' % (what, actual))
    else:
        failures.append('%s: %r, expected %r' % (what, actual, expected))
        print('  FAIL  %s: %r, expected %r' % (what, actual, expected))


def counted(method):
    def counting(self, *args):
        calls[type(self).__name__ + '.' + method.__name__] += 1
        return method(self, *args)
    return counting


class FakeType(_cdbext.Type):
    """A resolved type as PyValue::type() hands it out, or an unresolved one as
    cdbext.lookupType() does."""

    def __init__(self, name, code=TypeCode.Struct, size=8, module=0x1000, isResolved=True):
        self.typeName = name
        self.typeCode = code
        self.size = size
        self.moduleBase = module
        self.isResolved = isResolved

    @counted
    def name(self):
        return self.typeName

    @counted
    def code(self):
        return self.typeCode

    @counted
    def bitsize(self):
        return self.size * 8 if self.isResolved else 0

    @counted
    def module(self):
        return 'app' if self.isResolved else ''

    @counted
    def moduleId(self):
        return self.moduleBase if self.isResolved else 0

    @counted
    def resolved(self):
        return self.isResolved

    @counted
    def unresolvable(self):
        return not self.isResolved

    @counted
    def targetName(self):
        return self.typeName[:-1].strip() if self.typeName.endswith('*') else self.typeName

    @counted
    def fields(self):
        return []


class FakeValue(_cdbext.Value):
    def __init__(self, name, nativeType, address=0x2000, text='0', members=()):
        self.valueName = name
        self.nativeType = nativeType
        self.valueAddress = address
        self.text = text
        self.members = list(members)

    @counted
    def name(self):
        return self.valueName

    @counted
    def type(self):
        return self.nativeType

    @counted
    def bitsize(self):
        return self.nativeType.size * 8

    @counted
    def address(self):
        return self.valueAddress

    @counted
    def hasChildren(self):
        return False

    @counted
    def nativeDebuggerValue(self):
        return self.text

    @counted
    def children(self):
        return self.members

    @counted
    def childFromIndex(self, index):
        return self.members[index] if index < len(self.members) else None


dumper = cdbbridge.Dumper()

print('--- a value is asked for its type once ---')
calls.clear()
value = dumper.fromNativeValue(FakeValue('count', FakeType('int', TypeCode.Integral, 4), text='0n7'))
expect('type() round trips', calls['FakeValue.type'], 1)
expect('name() round trips', calls['FakeValue.name'], 1)
expect('the value is read from the text', value.ldata, (7).to_bytes(4, 'little'))

print('')
print('--- a resolved type is derived once ---')
listType = FakeType('QList<QObject *>', size=24)
calls.clear()
first = dumper.from_native_type(listType)
derivingCalls = dict(calls)
calls.clear()
second = dumper.from_native_type(listType)
expect('the same typeid', second, first)
expect('the size is not asked again', calls['FakeType.bitsize'], 0)
expect('the module is not asked again', calls['FakeType.module'], 0)
expect('the first derivation did ask for the size', derivingCalls.get('FakeType.bitsize', 0), 1)
expect('the size is cached', dumper.type_size(first), 24)

print('')
print('--- a pointer type is derived once as well ---')
pointerType = FakeType('QObject *', TypeCode.Pointer)
first = dumper.from_native_type(pointerType)
calls.clear()
expect('the same typeid', dumper.from_native_type(pointerType), first)
expect('nothing but the name is asked', dict(calls), {'FakeType.name': 1})

print('')
print('--- an unresolved type is derived again ---')
laterType = FakeType('Later', isResolved=False)
dumper.from_native_type(laterType)
calls.clear()
dumper.from_native_type(laterType)
expect('the type is looked at again', calls['FakeType.code'], 1)

print('')
print('--- a lookup is pointed at the module of the value being dumped ---')
lookups = []


def fake_lookup_type(name, module=0):
    lookups.append((name, module))
    return None


_cdbext.lookupType = fake_lookup_type
dumper.fromNativeValue(FakeValue('list', FakeType('QList<Foo>', module=0x7000)))
dumper.lookupNativeType('Foo')
dumper.lookupNativeType('Bar', 0x9000)
dumper.fromNativeValue(FakeValue('later', FakeType('Later', isResolved=False)))
dumper.lookupNativeType('Baz')
expect('the hint is the module of the last resolved value, an explicit module wins',
       lookups, [('Foo', 0x7000), ('Bar', 0x9000), ('Baz', 0x7000)])

print('')
print('--- the children of a value are walked once ---')
base = FakeValue('Base', FakeType('Base', size=4), address=0x3000)
member = FakeValue('count', FakeType('int', TypeCode.Integral, 4), address=0x3004, text='0n1')
parent = FakeValue('derived', FakeType('Derived', size=8), address=0x3000, members=[base, member])
calls.clear()
withBases = dumper.listNativeValueChildren(parent, True)
expect('children() is asked once', calls['FakeValue.children'], 1)
expect('childFromIndex() is not asked at all', calls['FakeValue.childFromIndex'], 0)
expect('each child is asked for its name once', calls['FakeValue.name'], 2)
expect('base and member are listed', [(v.name, v.isBaseClass) for v in withBases],
       [('Base', True), ('count', False)])
expect('the base class can be left out',
       [v.name for v in dumper.listNativeValueChildren(parent, False)], ['count'])

print('')
if failures:
    print('FAILED (%d):' % len(failures))
    for failure in failures:
        print('  %s' % failure)
    sys.exit(1)
print('PASSED')
