contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

# MOC_DIR = moc
DEFINES += QT_NO_STYLE_S60

debug {
  DEFINES += VIEWLOGGER
}

linux-g++-64 { 
    // note that -Werror = return-type \
        is \
        only \
        supported \
        with \
        gcc \
        >4.3
    QMAKE_CXXFLAGS += "-Werror=return-type -Werror=init-self"
    QMAKE_LFLAGS += "-rdynamic"
}
TMP_BAUHAUS_NO_OUTPUT = $$[BAUHAUS_NO_OUTPUT]
equals(TMP_BAUHAUS_NO_OUTPUT, true):DEFINES += QT_NO_DEBUG_OUTPUT \
    QT_NO_WARNING_OUTPUT
DEFINES += ENABLE_TEXT_VIEW \
    QWEAKPOINTER_ENABLE_ARROW

isEmpty($$(BAUHAUS_OUTPUT_IN_TEST)):DEFINES += QDEBUG_IN_TESTS WARNINGS_IN_TESTS
