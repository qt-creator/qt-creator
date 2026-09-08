# cdb types from another module

`app` defines a pointer to each of three structs, two of them only forward
declared there. `problib` defines those two and exports a function returning
each, which is what puts them in its PDB. `app` loads that library before it
stops in `afterLoad()`, so at the stop every type is available in some module.
It needs no Qt.

## Running

Open `CMakeLists.txt` with an MSVC kit, build, put a breakpoint on the `printf`
line of `afterLoad()` and start debugging. Add `libProbe`, `pointerToSecond`
and `appProbe` to the watch window. All three are pointers to a struct with one
`int`, so all three should expand to that member. That is the dumper path.
The last section has what the symbol group path does instead.

## What the three globals are for

`libProbe` is the case this fixture exists for: `LibProbe` is only in
`problib`, and the variable name matches the type name up to case.
`IDebugSymbols::GetTypeId()` asked for `LibProbe` in the `app` module has no
type to answer with, matches the variable instead and answers with its type,
`LibProbe*`, eight bytes and no fields. Taken as the type, that leaves the
value without children, and being resolved it is never looked up again.

`pointerToSecond` is the same shape without the name collision: `SecondProbe`
is only in `problib` too, no symbol of that name is in `app`, the lookup fails
there and moves on to the module that has the type.

`appProbe` is the collision without the missing type: `AppProbe` is in `app`,
so the type is what comes back and the variable never gets in the way.

## Without Creator

    cdb.exe -cf cmds.txt app.exe

with `cmds.txt`, `<build>` being a Qt Creator build:

    bu app!afterLoad
    g
    .load <build>\lib\qtcreatorcdbext64\qtcreatorcdbext.dll
    !qtcreatorcdbext.script t = cdbext.parseAndEvaluate('libProbe').type().target()
    !qtcreatorcdbext.script print(t.name(), t.module(), t.bitsize(), [f.name() for f in t.fields()])
    q

The type the dumper gets for the pointee, module and all:

    LibProbe problib 32 ['probeValue']

An extension that accepts what the app module answers reports that instead,
and no members come with it:

    LibProbe app 64 []

The type is only part of it. The value the dumper expands is the pointer's
entry in a symbol group, and dbgeng leaves that childless whatever the lookup
answered, so the extension casts the entry to the module-qualified type too.

## The symbol group path

The extension's `locals` command, which a cdb without Python reads locals
through and which the tracepoint capture uses, does not go through that
lookup. dbgeng leaves the pointer childless there, while `appProbe`, asked for
in the same call, does get its member:

    !qtcreatorcdbext.locals -t 1 -D -e watch.0,watch.1 -W -w watch.0 libProbe -w watch.1 appProbe 0
    {iname="watch.0",name="libProbe",type="struct LibProbe *",...,numchild="0"}
    {iname="watch.1",name="appProbe",type="struct AppProbe *",...,numchild="1",
     children=[{iname="watch.1.appValue",name="appValue",...}]}

Only a cast that names the module defining the type completes it:

    0:000> ?? (problib!LibProbe *)libProbe
    struct LibProbe * 0x00007ff6`fd70c000
       +0x000 probeValue       : 0n4711

The extension casts a childless pointer that way itself, on expansion,
after a lookup of the type over the loaded modules, so the call above answers

    {iname="watch.0",name="libProbe",type="struct LibProbe *",...,numchild="1",
     children=[{iname="watch.0.probeValue",name="probeValue",...}]}

Drop the `-e` and the children stay away, the count being a guess: the lookup
asks every module and makes the ones whose symbols are deferred load them, and
on a process with a few hundred modules that is seconds, too much to spend on a
pointer nobody opened.

The cdb backend of `tst_backends` does not read locals here. `CdbImpl` asks
for them as `theDumper.fetchVariables`, so the cdb rows of
`resolvesATypeArrivingWithALaterLibrary()` and
`readsALocalOfATypeOnlyALibraryDefines()` exercise the dumper path only.
`expandsAPointerToALibraryTypeInASymbolGroup()` sends the call above, as typed
into the debugger console, to cover this one.
