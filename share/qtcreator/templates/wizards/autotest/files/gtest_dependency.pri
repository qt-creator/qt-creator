isEmpty(GOOGLETEST_DIR):GOOGLETEST_DIR=$$(GOOGLETEST_DIR)

isEmpty(GOOGLETEST_DIR) {
    warning("Using googletest src dir specified at Qt Creator wizard")
    message("set GOOGLETEST_DIR as environment variable or qmake variable to get rid of this message")
    GOOGLETEST_DIR = %{GTestRepository}
}

!isEmpty(GOOGLETEST_DIR): {
    GTEST_SRCDIR = $$GOOGLETEST_DIR/googletest
    GMOCK_SRCDIR = $$GOOGLETEST_DIR/googlemock
}

requires(exists($$GTEST_SRCDIR):exists($$GMOCK_SRCDIR))

!exists($$GOOGLETEST_DIR):message("No googletest src dir found - set GOOGLETEST_DIR to enable.")

@if "%{GTestCXX11}" == "true"
DEFINES += \\
    GTEST_LANG_CXX11
@endif

INCLUDEPATH *= \\
    $$GTEST_SRCDIR \\
    $$GTEST_SRCDIR/include \\
    $$GMOCK_SRCDIR \\
    $$GMOCK_SRCDIR/include

SOURCES += \\
    $$GTEST_SRCDIR/src/gtest-all.cc \\
    $$GMOCK_SRCDIR/src/gmock-all.cc
