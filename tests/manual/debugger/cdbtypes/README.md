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

## The symbol group path

The extension's `locals` command, which the cdb backend of `tst_backends` and
a cdb without Python read locals through, does not go through that lookup, and
dbgeng leaves the pointer childless there:

    !qtcreatorcdbext.locals -t 1 -D -e watch.0,watch.1 -W -w watch.0 libProbe -w watch.1 appProbe 0
    {iname="watch.0",name="libProbe",type="struct LibProbe *",...,numchild="0"}
    {iname="watch.1",name="appProbe",type="struct AppProbe *",...,numchild="1",
     children=[{iname="watch.1.appValue",name="appValue",...}]}

`appProbe`, asked for in the same call, does get its member. dbgeng completes
the type for a cast that names the module:

    0:000> ?? (problib!LibProbe *)libProbe
    struct LibProbe * 0x00007ff6`fd70c000
       +0x000 probeValue       : 0n4711

which is what a fix on that path would have to arrange, and why
`resolvesATypeArrivingWithALaterLibrary()` in `tst_backends` keeps its cdb row
disabled.
