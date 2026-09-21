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

memory = bytearray(0x200)
_cdbext.readRawMemory = lambda address, size: bytes(memory[address - 0x3000:address - 0x3000 + size])
symbolsAdded = []


def fake_create_value(address, nativeType):
    symbolsAdded.append(address)
    return FakeValue('*', nativeType, address=address)


_cdbext.createValue = fake_create_value
intType = FakeType('int', TypeCode.Integral, 4)
uintType = FakeType('unsigned int', TypeCode.Integral, 4)


def listed(members):
    return [(m.name, int.from_bytes(m.ldata, 'little')) for m in members]


print('')
print('--- the layout of a struct is recorded from its first value ---')
memory[0x000:0x008] = (3).to_bytes(4, 'little') + (2).to_bytes(4, 'little')
memory[0x100:0x108] = (5).to_bytes(4, 'little') + (6).to_bytes(4, 'little')
fooType = FakeType('Foo', size=8)
foo = dumper.fromNativeValue(FakeValue('f', fooType, address=0x3000, members=[
    FakeValue('a', intType, address=0x3000, text='0n3'),
    FakeValue('b', intType, address=0x3004, text='0n2')]))
expect('the first value is listed from the symbol group',
       listed(dumper.value_members(foo, True)), [('a', 3), ('b', 2)])
fields = dumper.type_fields_cache.get(foo.typeid, None)
expect('and its layout is recorded',
       [(f.name, f.bitpos, f.bitsize) for f in fields] if fields else None,
       [('a', 0, 32), ('b', 32, 32)])
another = dumper.createValue(0x3100, 'Foo')
expect('the next value of the type is read from memory',
       listed(dumper.value_members(another, True)), [('a', 5), ('b', 6)])
expect('without a symbol added for it', symbolsAdded, [])

print('')
print('--- a layout the memory would misreport is not recorded ---')
memory[0x010:0x014] = (0x1a).to_bytes(4, 'little')      # x:3 = 2 and y:4 = 3 in one unit
bits = dumper.fromNativeValue(FakeValue('s', FakeType('Bits', size=4), address=0x3010, members=[
    FakeValue('x', uintType, address=0x3010, text='2'),
    FakeValue('y', uintType, address=0x3010, text='3')]))
dumper.value_members(bits, True)
expect('members sharing storage', dumper.type_fields_cache.get(bits.typeid, None), None)
# Both bits happen to be zero, so what the engine prints is what memory holds;
# only the shared storage gives them away.
zeroBits = dumper.fromNativeValue(FakeValue('z', FakeType('ZeroBits', size=4), address=0x3040, members=[
    FakeValue('x', uintType, address=0x3040, text='0'),
    FakeValue('y', uintType, address=0x3040, text='0')]))
dumper.value_members(zeroBits, True)
expect('members sharing storage, all zero', dumper.type_fields_cache.get(zeroBits.typeid, None), None)
memory[0x020:0x024] = (0x101).to_bytes(4, 'little')     # flag:1 = 1, with a bit set next to it
lone = dumper.fromNativeValue(FakeValue('l', FakeType('Lone', size=4), address=0x3020, members=[
    FakeValue('flag', uintType, address=0x3020, text='1')]))
dumper.value_members(lone, True)
expect('a printed value memory does not hold', dumper.type_fields_cache.get(lone.typeid, None), None)
expect('and the type is not tried again', lone.typeid in dumper.type_layout_rejected, True)
memory[0x030:0x034] = (1).to_bytes(4, 'little')
withEnum = dumper.fromNativeValue(FakeValue('e', FakeType('WithEnum', size=4), address=0x3030, members=[
    FakeValue('kind', FakeType('Kind', TypeCode.Enum, 4), address=0x3030, text='V2 (0n1)')]))
dumper.value_members(withEnum, True)
expect('an enum member', dumper.type_fields_cache.get(withEnum.typeid, None), None)
withLater = dumper.fromNativeValue(FakeValue('u', FakeType('WithLater', size=4), address=0x3030, members=[
    FakeValue('later', FakeType('Later', size=4, isResolved=False), address=0x3030, text='{...}')]))
dumper.value_members(withLater, True)
expect('a member of a type the engine could not resolve',
       dumper.type_fields_cache.get(withLater.typeid, None), None)

print('')
print('--- a value whose memory cannot be read goes to the symbol group ---')
symbolsAdded.clear()
unreadable = dumper.createValue(0x9000, 'Foo')
expect('the members are listed by the debugger', listed(dumper.value_members(unreadable, True)), [])
expect('with a symbol added for the value', symbolsAdded, [0x9000])

print('')
print('--- a value without an address goes to the symbol group ---')
inRegister = dumper.fromNativeValue(FakeValue('r', fooType, address=None, members=[
    FakeValue('a', intType, address=None, text='0n8'),
    FakeValue('b', intType, address=None, text='0n9')]))
expect('the members are listed by the debugger',
       listed(dumper.value_members(inRegister, True)), [('a', 8), ('b', 9)])

print('')
print('--- what a layout check found goes with the type that stops being usable ---')
memory[0x0a0:0x0a8] = (1).to_bytes(4, 'little') + (2).to_bytes(4, 'little')
goneType = FakeType('Gone', size=8)
gone = dumper.fromNativeValue(FakeValue('g', goneType, address=0x30a0, members=[
    FakeValue('a', intType, address=0x30a0, text='0n1'),
    FakeValue('b', intType, address=0x30a4, text='0n2')]))
dumper.value_members(gone, True)
sharedType = FakeType('Shared', size=4)
shared = dumper.fromNativeValue(FakeValue('s', sharedType, address=0x30b0, members=[
    FakeValue('x', uintType, address=0x30b0, text='0'),
    FakeValue('y', uintType, address=0x30b0, text='0')]))
dumper.value_members(shared, True)
expect('a layout is recorded', gone.typeid in dumper.type_fields_cache, True)
expect('and another type rejected', shared.typeid in dumper.type_layout_rejected, True)
goneType.isResolved = False
sharedType.isResolved = False
dumper.cached_nativetype(gone.typeid)
dumper.cached_nativetype(shared.typeid)
expect('the layout is dropped', gone.typeid in dumper.type_fields_cache, False)
expect('and so is the rejection', shared.typeid in dumper.type_layout_rejected, False)

print('')
if failures:
    print('FAILED (%d):' % len(failures))
    for failure in failures:
        print('  %s' % failure)
    sys.exit(1)
print('PASSED')
