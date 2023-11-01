isEmpty(GOOGLETEST_DIR):GOOGLETEST_DIR=$$(GOOGLETEST_DIR)

isEmpty(GOOGLETEST_DIR) {
    GOOGLETEST_DIR = "%{GTestBaseFolder}"
    !isEmpty(GOOGLETEST_DIR) {
        warning("Using googletest src dir specified at Qt Creator wizard")
        message("set GOOGLETEST_DIR as environment variable or qmake variable to get rid of this message")
    }
}

@if "%{TestFrameWork}" == "GTest"
!isEmpty(GOOGLETEST_DIR): {
    GTEST_SRCDIR = "$$GOOGLETEST_DIR/googletest"
    GMOCK_SRCDIR = "$$GOOGLETEST_DIR/googlemock"
} else: unix {
    exists(/usr/src/gtest):GTEST_SRCDIR=/usr/src/gtest
    exists(/usr/src/gmock):GMOCK_SRCDIR=/usr/src/gmock
    !isEmpty(GTEST_SRCDIR): message("Using gtest from system")
}

requires(exists($$GTEST_SRCDIR):exists($$GMOCK_SRCDIR))

!isEmpty(GTEST_SRCDIR) {
    INCLUDEPATH *= \\
        $$GTEST_SRCDIR \\
        $$GTEST_SRCDIR/include

    SOURCES += \\
        $$GTEST_SRCDIR/src/gtest-all.cc
}

!isEmpty(GMOCK_SRCDIR) {
    INCLUDEPATH *= \\
        $$GMOCK_SRCDIR \\
        $$GMOCK_SRCDIR/include

    SOURCES += \\
        $$GMOCK_SRCDIR/src/gmock-all.cc
}
@endif
@if "%{TestFrameWork}" == "GTest_dyn"
!isEmpty(GOOGLETEST_DIR): {
    INCLUDEPATH *= "$$GOOGLETEST_DIR/include"

    LIBS *= -L"$$GOOGLETEST_DIR/lib" -lgtest -lgmock
} else {
    LIBS *= -lgtest -lgmock
}
@endif
