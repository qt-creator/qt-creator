isEmpty(CATCH2_INSTALL_DIR):CATCH2_INSTALL_DIR=$$(CATCH2_INSTALL_DIR)

isEmpty(CATCH2_INSTALL_DIR) {
    CATCH2_INSTALL_DIR = "%{CatchInstallDir}" # set by QC
    !isEmpty(CATCH2_INSTALL_DIR) {
        warning("Using Catch2 installation specified at Qt Creator wizard.")
        message("Set CATCH2_INSTALL_DIR as environment variable or qmake variable to get rid of this message")
    } else {
        message("Using Catch2 from system - set CATCH2_INSTALL_DIR is it cannot be found automatically.")
    }
}

!isEmpty(CATCH2_INSTALL_DIR): {
    INCLUDEPATH *= "$$CATCH2_INSTALL_DIR/include"

    equals(CATCH2_MAIN, 0): LIBS *= -L"$$CATCH2_INSTALL_DIR/lib" -lCatch2Main -lCatch2
    else: LIBS *= -L"$$CATCH2_INSTALL_DIR/lib" -lCatch2
} else {
    equals(CATCH2_MAIN, 0): LIBS *= -lCatch2Main -lCatch2
    else: LIBS *= -lCatch2
}
