DEFINES += CPPEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)
HEADERS += cppeditorplugin.h \
    cppautocompleter.h \
    cppclasswizard.h \
    cppeditorconstants.h \
    cppeditorenums.h \
    cppeditor_global.h \
    cppeditor.h \
    cppelementevaluator.h \
    cppfilewizard.h \
    cppfunctiondecldeflink.h \
    cpphighlighter.h \
    cpphoverhandler.h \
    cppoutline.h \
    cppquickfixassistant.h \
    cppquickfixes.h \
    cppquickfix.h \
    cppsnippetprovider.h \
    cpptypehierarchy.h

SOURCES += cppeditorplugin.cpp \
    cppautocompleter.cpp \
    cppclasswizard.cpp \
    cppeditor.cpp \
    cppelementevaluator.cpp \
    cppfilewizard.cpp \
    cppfunctiondecldeflink.cpp \
    cpphighlighter.cpp \
    cpphoverhandler.cpp \
    cppoutline.cpp \
    cppquickfixassistant.cpp \
    cppquickfix.cpp \
    cppquickfixes.cpp \
    cppsnippetprovider.cpp \
    cpptypehierarchy.cpp

RESOURCES += cppeditor.qrc
OTHER_FILES += CppEditor.mimetypes.xml

equals(TEST, 1) {
    SOURCES += \
        cppquickfix_test.cpp \
        cppdoxygen_test.cpp \
        fileandtokenactions_test.cpp

    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
