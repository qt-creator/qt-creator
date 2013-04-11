DEFINES += CPPEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)
HEADERS += cppplugin.h \
    cppeditor.h \
    cpphighlighter.h \
    cpphoverhandler.h \
    cppfilewizard.h \
    cppeditorconstants.h \
    cppeditorenums.h \
    cppeditor_global.h \
    cppclasswizard.h \
    cppoutline.h \
    cpptypehierarchy.h \
    cppelementevaluator.h \
    cppautocompleter.h \
    cppsnippetprovider.h \
    cppquickfixassistant.h \
    cppquickfix.h \
    cppquickfixes.h \
    cppfunctiondecldeflink.h

SOURCES += cppplugin.cpp \
    cppeditor.cpp \
    cpphighlighter.cpp \
    cpphoverhandler.cpp \
    cppfilewizard.cpp \
    cppclasswizard.cpp \
    cppquickfixes.cpp \
    cppoutline.cpp \
    cpptypehierarchy.cpp \
    cppelementevaluator.cpp \
    cppautocompleter.cpp \
    cppsnippetprovider.cpp \
    cppquickfixassistant.cpp \
    cppquickfix.cpp \
    cppfunctiondecldeflink.cpp

RESOURCES += cppeditor.qrc
OTHER_FILES += CppEditor.mimetypes.xml

equals(TEST, 1) {
    SOURCES += \
        cppquickfix_test.cpp \
        cppdoxygen_test.cpp \
        fileandtokenactions_test.cpp

    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
