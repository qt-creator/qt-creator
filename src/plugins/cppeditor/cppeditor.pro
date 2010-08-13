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
    cpprefactoringchanges.h \
    cppchecksymbols.h \
    cppsemanticinfo.h \
    cppoutline.h \
    cppdeclfromdef.h \
    cpplocalsymbols.h \
    cpptypehierarchy.h \
    cppelementevaluator.h
SOURCES += cppplugin.cpp \
    cppeditor.cpp \
    cpphighlighter.cpp \
    cpphoverhandler.cpp \
    cppfilewizard.cpp \
    cppclasswizard.cpp \
    cppquickfix.cpp \
    cppquickfixes.cpp \
    cpprefactoringchanges.cpp \
    cppchecksymbols.cpp \
    cppsemanticinfo.cpp \
    cppoutline.cpp \
    cppdeclfromdef.cpp \
    cpplocalsymbols.cpp \
    cpptypehierarchy.cpp \
    cppelementevaluator.cpp
RESOURCES += cppeditor.qrc
OTHER_FILES += CppEditor.pluginspec \
    CppEditor.mimetypes.xml
