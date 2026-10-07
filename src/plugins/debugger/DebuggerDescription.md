Debug an application to see what happens inside it while it runs or when it
crashes. The debugger extension is a graphical front end to native debuggers
and to the QML and Python debuggers.

![The debugger stopped at a breakpoint](https://qtccache.qt.io/images/qtcreator-extension-debugger.webp)

*An application stopped at a breakpoint, with its local variables and call stack*

## Features

- Debug with GDB, LLDB, or CDB, or debug QML and Python code. Qt Creator
  selects a suitable debugger for each kit
- Set breakpoints, including data breakpoints that stop when data is read or
  written
- Step through the application line by line or instruction by instruction,
  and follow the call stack
- Inspect local variables, evaluate expressions, and view registers, memory,
  modules, and disassembled code
- View Qt and standard library types in a readable form with debugging
  helpers
- Attach to running applications, inspect core files, and debug on remote
  devices

## You also need:

- A debugger built with Python scripting support: GDB 8.0 or later, LLDB, or
  the Debugging Tools for Windows (CDB)
