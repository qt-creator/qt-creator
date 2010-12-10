TEMPLATE = lib
TARGET = CppEditor
DEFINES += CPPEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/libs/cplusplus/cplusplus.pri)
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
    cppquickfix.h \
    cppchecksymbols.h \
    cppsemanticinfo.h \
    cppoutline.h \
    cppinsertdecldef.h \
    cpplocalsymbols.h \
    cpptypehierarchy.h \
    cppelementevaluator.h \
    cppquickfixcollector.h \
    cppqtstyleindenter.h \
    cppautocompleter.h \
    cppcompleteswitch.h \
    cppsnippetprovider.h \
    cppinsertqtpropertymembers.h

SOURCES += cppplugin.cpp \
    cppeditor.cpp \
    cpphighlighter.cpp \
    cpphoverhandler.cpp \
    cppfilewizard.cpp \
    cppclasswizard.cpp \
    cppquickfix.cpp \
    cppquickfixes.cpp \
    cppchecksymbols.cpp \
    cppsemanticinfo.cpp \
    cppoutline.cpp \
    cppinsertdecldef.cpp \
    cpplocalsymbols.cpp \
    cpptypehierarchy.cpp \
    cppelementevaluator.cpp \
    cppquickfixcollector.cpp \
    cppqtstyleindenter.cpp \
    cppautocompleter.cpp \
    cppcompleteswitch.cpp \
    cppsnippetprovider.cpp \
    cppinsertqtpropertymembers.cpp

RESOURCES += cppeditor.qrc
OTHER_FILES += CppEditor.mimetypes.xml
