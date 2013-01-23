TEMPLATE = lib
TARGET = CppEditor
DEFINES += CPPEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)
include(cppeditor_dependencies.pri)
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
    cppinsertdecldef.h \
    cpptypehierarchy.h \
    cppelementevaluator.h \
    cppautocompleter.h \
    cppcompleteswitch.h \
    cppsnippetprovider.h \
    cppinsertqtpropertymembers.h \
    cppquickfixassistant.h \
    cppquickfix.h \
    cppfunctiondecldeflink.h

SOURCES += cppplugin.cpp \
    cppeditor.cpp \
    cpphighlighter.cpp \
    cpphoverhandler.cpp \
    cppfilewizard.cpp \
    cppclasswizard.cpp \
    cppquickfixes.cpp \
    cppoutline.cpp \
    cppinsertdecldef.cpp \
    cpptypehierarchy.cpp \
    cppelementevaluator.cpp \
    cppautocompleter.cpp \
    cppcompleteswitch.cpp \
    cppsnippetprovider.cpp \
    cppinsertqtpropertymembers.cpp \
    cppquickfixassistant.cpp \
    cppquickfix.cpp \
    cppfunctiondecldeflink.cpp

RESOURCES += cppeditor.qrc
OTHER_FILES += CppEditor.mimetypes.xml

equals(TEST, 1) {
    SOURCES += \
        cppquickfix_test.cpp

    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
