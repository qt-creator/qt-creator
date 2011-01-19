# Detect presence of "Debugging Tools For Windows"
# in case MS VS compilers are used.

CDB_PATH=""
win32 {
    contains(QMAKE_CXX, cl) {
        CDB_PATH="$$(CDB_PATH)"
        isEmpty(CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows/sdk"
        !exists($$CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows (x86)/sdk"
        !exists($$CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows (x64)/sdk"
        !exists($$CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows 64-bit/sdk"
        !exists($$CDB_PATH):CDB_PATH="$$(ProgramW6432)/Debugging Tools For Windows (x86)/sdk"
        !exists($$CDB_PATH):CDB_PATH="$$(ProgramW6432)/Debugging Tools For Windows (x64)/sdk"
        !exists($$CDB_PATH):CDB_PATH="$$(ProgramW6432)/Debugging Tools For Windows 64-bit/sdk"
        !exists($$CDB_PATH)::CDB_PATH=""
    }
}
