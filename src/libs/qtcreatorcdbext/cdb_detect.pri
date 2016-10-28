# Detect presence of "Debugging Tools For Windows"
# in case MS VS compilers are used.

CDB_PATH=""
msvc {
    CDB_PATH="$$(CDB_PATH)"
    isEmpty(CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows/sdk"
    !exists($$CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows (x86)/sdk"
    !exists($$CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows (x64)/sdk"
    !exists($$CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows 64-bit/sdk"
    !exists($$CDB_PATH):CDB_PATH="$$(ProgramW6432)/Debugging Tools For Windows (x86)/sdk"
    !exists($$CDB_PATH):CDB_PATH="$$(ProgramW6432)/Debugging Tools For Windows (x64)/sdk"
    !exists($$CDB_PATH):CDB_PATH="$$(ProgramW6432)/Debugging Tools For Windows 64-bit/sdk"
#   Starting from Windows SDK 8, the headers and libs are under 'ProgramFiles (x86)'.
#   The libraries are under 'ProgramFiles'as well, so, check for existence of 'inc'.
#   32bit qmake:
    !exists($$CDB_PATH/inc):CDB_PATH="$$(ProgramFiles)/Windows Kits/8.0/Debuggers"
    !exists($$CDB_PATH/inc):CDB_PATH="$$(ProgramFiles)/Windows Kits/8.1/Debuggers"
    !exists($$CDB_PATH/inc):CDB_PATH="$$(ProgramFiles)/Windows Kits/10/Debuggers"
#   64bit qmake:
    !exists($$CDB_PATH/inc):CDB_PATH="$$(ProgramFiles) (x86)/Windows Kits/8.0/Debuggers"
    !exists($$CDB_PATH/inc):CDB_PATH="$$(ProgramFiles) (x86)/Windows Kits/8.1/Debuggers"
    !exists($$CDB_PATH/inc):CDB_PATH="$$(ProgramFiles) (x86)/Windows Kits/10/Debuggers"
    !exists($$CDB_PATH/inc):CDB_PATH=""
}
