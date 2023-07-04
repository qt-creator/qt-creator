# Sqlite tests
extend_qtc_test(unittest
SOURCES
    lastchangedrowid-test.cpp
    sqlitealgorithms-test.cpp
    sqliteindex-test.cpp
    sqliteteststatement.h
    sqlitetransaction-test.cpp
    sqlitecolumn-test.cpp
    sqlitedatabasebackend-test.cpp
    sqlitedatabase-test.cpp
    sqlitefunctionregistry-test.cpp
    sqlitesessions-test.cpp
    sqlitestatement-test.cpp
    sqlitetable-test.cpp
    sqlstatementbuilder-test.cpp
    createtablesqlstatementbuilder-test.cpp
    sqlitevalue-test.cpp
)

unittest_copy_data_folder()
