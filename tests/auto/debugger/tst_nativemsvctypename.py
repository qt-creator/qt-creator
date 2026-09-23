# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

r"""Checks that the cdb bridge asks the symbol reader for a name it can resolve.

cdbbridge.native_msvc_type_name() reverses the type name collapsing done by
DumperBase.sanitize_type_name(), and DumperBase.type_nativetype() offers the
spellings until one of them describes the type.

The internal type key drops the spaces around '&*<>,' that MSVC, and therefore
every PDB, actually emits. Querying CDB for the collapsed spelling makes dbghelp
call GetTypeId() once per loaded module, which on a process with a few hundred
modules takes tens of seconds and looks like a hang in the Locals view.

Run the self-contained checks:

    python tst_nativemsvctypename.py

Additionally validate against a real PDB's type table (needs the Debugging Tools
for Windows):

    dbh.exe <path>\Qt6Cored.dll etypes > types.txt
    python tst_nativemsvctypename.py types.txt
"""

import os
import re
import sys
import types

DEBUGGER_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            '..', '..', '..', 'share', 'qtcreator', 'debugger')
sys.path.insert(0, os.path.normpath(DEBUGGER_DIR))

# cdbext is provided by the CDB extension and cannot be imported here.
_cdbext = types.ModuleType('cdbext')
_cdbext.Type = type('Type', (), {})
_cdbext.Value = type('Value', (), {})
_cdbext.lookupType = lambda name, module=0: None
_cdbext.listOfModules = lambda: []
_cdbext.pointerSize = lambda: 8
sys.modules['cdbext'] = _cdbext

import cdbbridge
from utils import TypeCode
from cdbbridge import native_msvc_type_name
from dumper import DumperBase

failures = []


def sanitize(name):
    return DumperBase.sanitize_type_name(None, name)


def check(pdb_spelling):
    """A PDB spelling must survive collapsing and reconstruction unchanged."""
    collapsed = sanitize(pdb_spelling)
    restored = native_msvc_type_name(collapsed)
    if restored != pdb_spelling:
        failures.append('%r -> %r -> %r' % (pdb_spelling, collapsed, restored))
    return restored == pdb_spelling


# Spellings taken verbatim from Qt6Cored.pdb and Qt5Cored.pdb.
SPELLINGS = [
    'QList<QObject *>',                          # the one that caused QTCREATORBUG freeze
    'QObject *',
    'void * *',
    'char const *',
    'QString const &',
    'char const * &',
    'QList<QList<int> >',
    'QList<QPointer<QObject> >',
    'char [13]',
    'QAtomicPointer<QCalendarBackend const >',
    'std::pair<QString const ,QCalendarBackend * const>',
    'QFlags<enum <unnamed-enum-SmallValueBitLength> >',
    'QObject * (__cdecl*)(void)',
    'void (__cdecl*)(void *)',
    'bool (__cdecl*)(void * *)',
    'std::pair<QTypedArrayData<QObject * (__cdecl*)(void)> *,QObject * (__cdecl**)(void)>',
    'QMap<int,QVariant>',
    'QHash<QString,QVariant>::const_iterator',
    'QDirListing::const_iterator',                # 'const' inside an identifier
    'constructor_type',
    'sljit_const',
]

print('--- known PDB spellings ---')
for spelling in SPELLINGS:
    print('  %-5s %s' % ('PASS' if check(spelling) else 'FAIL', spelling))

print('')
print('--- the reconstruction runs over its own output ---')
# type_name() answers with the MSVC spelling for every typeid that came off a
# native type, so native_msvc_type_name() is applied to names it has already
# produced. Applying it twice has to be a no-op.
not_idempotent = [(spelling, native_msvc_type_name(spelling))
                  for spelling in SPELLINGS
                  if native_msvc_type_name(spelling) != spelling]
for spelling, restored in not_idempotent:
    failures.append('%r is changed by a second pass into %r' % (spelling, restored))
    print('  FAIL  %s -> %s' % (spelling, restored))
if not not_idempotent:
    print('  PASS  %d spellings unchanged by a second pass' % len(SPELLINGS))

print('')
print('--- a name registered with a space after the comma ---')
# register_struct('@QMap<@QString, @QVariant>') keeps that space through
# sanitize_type_name(), and no PDB spells QVariantMap that way.
registered_key = sanitize('@QMap<@QString, @QVariant>')
reconstructed = native_msvc_type_name(registered_key)
if reconstructed != 'QMap<QString,QVariant>':
    failures.append('registered name: %r -> %r' % (registered_key, reconstructed))
    print('  FAIL  %r -> %r' % (registered_key, reconstructed))
else:
    print('  PASS  %r -> %r' % (registered_key, reconstructed))

print('')
print('--- the collapsed form must not be returned unchanged ---')
collapsed = sanitize('QList<QObject *>')
if collapsed != 'QList<QObject*>':
    failures.append('sanitize_type_name no longer collapses: %r' % collapsed)
    print('  FAIL  sanitize_type_name() gave %r' % collapsed)
else:
    print('  PASS  sanitize_type_name() gives %r' % collapsed)

known_names = set()
lookups = []


class FakeNativeType(_cdbext.Type):
    """What cdbext.lookupType() hands out: a name it parsed, not one it looked up.

    PyType::resolve() runs on the first access that needs the type id - moduleId()
    is one - and only then does the type know whether a module has its symbols.
    Until then it is neither resolved nor unresolvable.
    """

    def __init__(self, name):
        self.typeName = name
        self.isResolved = None

    def name(self):
        return self.typeName

    def moduleId(self):
        self.isResolved = self.typeName in known_names
        return 1 if self.isResolved else 0

    def resolved(self):
        return self.isResolved is True

    def unresolvable(self):
        return self.isResolved is False

    def bitsize(self):
        return 64 if self.isResolved else 0

    def code(self):
        return TypeCode.Struct

    def targetName(self):
        return self.typeName

    def module(self):
        return 'app' if self.isResolved else ''

    def fields(self):
        return []


def fake_lookup_type(name, module=0):
    lookups.append(name)
    return FakeNativeType(name)


_cdbext.lookupType = fake_lookup_type


def expect(what, actual, expected):
    if actual == expected:
        print('  PASS  %s: %r' % (what, actual))
    else:
        failures.append('%s: %r, expected %r' % (what, actual, expected))
        print('  FAIL  %s: %r, expected %r' % (what, actual, expected))


dumper = cdbbridge.Dumper()

print('')
print('--- the candidate a lookup settles on ---')
# Only the collapsed spelling is in this symbol table, so the reconstructed one is
# offered first, answered, and rejected because it does not resolve.
known_names.add('QList<QObject*>')
typeid = dumper.typeid_for_string('QList<QObject*>')
del lookups[:]
native_type = dumper.type_nativetype(typeid)
expect('both spellings offered', lookups, ['QList<QObject *>', 'QList<QObject*>'])
expect('the resolved one wins', native_type.name(), 'QList<QObject*>')
expect('and is cached', dumper.type_nativetype_cache.get(typeid, None), native_type)

print('')
print('--- a type whose symbols have not arrived yet ---')
later = dumper.typeid_for_string('Later<int*>')
del lookups[:]
unusable = dumper.type_nativetype(later)
expect('no candidate resolves', dumper.nativeTypeIsUsable(unusable), False)
expect('the unusable answer is not kept', dumper.type_nativetype_cache.get(later, None), None)
expect('nor is a size derived from it',
       (dumper.type_size(later), dumper.type_size_cache.get(later, None)), (0, None))
expect('nor an alignment',
       (dumper.type_alignment(later), dumper.type_alignment_cache.get(later, None)), (8, None))
known_names.add('Later<int *>')
del lookups[:]
expect('the later module load is picked up', dumper.type_nativetype(later).name(), 'Later<int *>')
expect('and so is the size', dumper.type_size(later), 8)

print('')
print('--- a registered type that no module knows ---')
# register_struct() seeds size and code for the types the dumpers know by
# heart, once the Qt version is known. Asking the symbol reader for one in
# vain must leave the seeds alone.
dumper.setQtVersionAtLeast6(True)
registered = dumper.typeid_for_string('@QList<@QRect>')
seeded = (dumper.type_size_cache.get(registered, None), dumper.type_code_cache.get(registered, None))
expect('the type is seeded', seeded[0] is not None and seeded[1] is not None, True)
dumper.type_nativetype(registered)
dumper.lookupType('@QList<@QRect>')
dumper.type_nativetype(registered)
expect('the seeds survive the lookups',
       (dumper.type_size_cache.get(registered, None), dumper.type_code_cache.get(registered, None)), seeded)
expect('and the size is the seeded one', dumper.type_size(registered), seeded[0])

print('')
print('--- a probe mints no typeid ---')
known_names.add('Probe<int *>')
minted = len(dumper.typeid_cache)
expect('a spelling the reader resolves is known', dumper.type_name_is_known('Probe<int*>'), True)
expect('one it does not is not', dumper.type_name_is_known('Nowhere<int*>'), False)
expect('and neither probe minted a typeid', len(dumper.typeid_cache), minted)

print('')
print('--- the spellings that are recorded ---')
hand_written = dumper.typeid_for_string('@QMap<@QString, @QVariant>')
expect('a name a dumper wrote is not one the reader gave',
       dumper.type_nativename_cache.get(hand_written, None), None)
noted = dumper.typeid_for_string('QHash<QString,QObject*>')
dumper.note_native_type_name(noted, 'QHash<QString,QObject *>')
expect('a name the reader gave is recorded',
       dumper.type_nativename_cache.get(noted, None), 'QHash<QString,QObject *>')
expect('and is offered first',
       next(iter(dumper.native_type_name_candidates(noted))), 'QHash<QString,QObject *>')
anonymous = dumper.typeid_for_string('s{x:int}{p:QObject*}')
dumper.note_native_type_name(anonymous, 's{x:int}{p:QObject *}')
expect('an anonymous struct shape is not a type name',
       dumper.type_nativename_cache.get(anonymous, None), None)

for path in sys.argv[1:]:
    names = set()
    with open(path, encoding='utf-8', errors='replace') as corpus:
        for line in corpus:
            match = re.match(r'\s*0x[0-9a-f]+\s*:\s(.+?)\s*$', line)
            # Decorated names ('?$Method@...') are mangled, not type names, and
            # sanitize_type_name() eats their '@' separators.
            if match and '@' not in match.group(1):
                names.add(match.group(1))

    bad = [name for name in sorted(names)
           if native_msvc_type_name(sanitize(name)) != name]
    print('')
    print('--- %s: %d names, %d not reproduced ---'
          % (os.path.basename(path), len(names), len(bad)))
    for name in bad:
        # '>>' in a nested-name is operator>>, not a template close. Telling those
        # apart needs a real C++ name parser; such names are lambda scopes that are
        # never looked up.
        if '::>>' in name or '::<<' in name:
            print('  known residue: %s' % name)
        else:
            failures.append('corpus: %s' % name)
            print('  FAIL: %s' % name)

print('')
if failures:
    print('FAILED (%d):' % len(failures))
    for failure in failures:
        print('  %s' % failure)
    sys.exit(1)
print('PASSED')
