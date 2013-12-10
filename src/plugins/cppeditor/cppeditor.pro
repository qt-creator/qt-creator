DEFINES += CPPEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += \
    cppautocompleter.h \
    cppclasswizard.h \
    cppeditor.h \
    cppeditor_global.h \
    cppeditorconstants.h \
    cppeditorenums.h \
    cppeditorplugin.h \
    cppelementevaluator.h \
    cppfilewizard.h \
    cppfollowsymbolundercursor.h \
    cppfunctiondecldeflink.h \
    cpphighlighter.h \
    cpphighlighterfactory.h \
    cpphoverhandler.h \
    cppincludehierarchy.h \
    cppincludehierarchyitem.h \
    cppincludehierarchymodel.h \
    cppincludehierarchytreeview.h \
    cppoutline.h \
    cpppreprocessordialog.h \
    cppquickfix.h \
    cppquickfixassistant.h \
    cppquickfixes.h \
    cppsnippetprovider.h \
    cpptypehierarchy.h \
    cppvirtualfunctionassistprovider.h \
    cppvirtualfunctionproposalitem.h

SOURCES += \
    cppautocompleter.cpp \
    cppclasswizard.cpp \
    cppeditor.cpp \
    cppeditorplugin.cpp \
    cppelementevaluator.cpp \
    cppfilewizard.cpp \
    cppfollowsymbolundercursor.cpp \
    cppfunctiondecldeflink.cpp \
    cpphighlighter.cpp \
    cpphighlighterfactory.cpp \
    cpphoverhandler.cpp \
    cppincludehierarchy.cpp \
    cppincludehierarchyitem.cpp \
    cppincludehierarchymodel.cpp \
    cppincludehierarchytreeview.cpp \
    cppoutline.cpp \
    cpppreprocessordialog.cpp \
    cppquickfix.cpp \
    cppquickfixassistant.cpp \
    cppquickfixes.cpp \
    cppsnippetprovider.cpp \
    cpptypehierarchy.cpp \
    cppvirtualfunctionassistprovider.cpp \
    cppvirtualfunctionproposalitem.cpp

FORMS += \
    cpppreprocessordialog.ui

RESOURCES += \
    cppeditor.qrc

equals(TEST, 1) {
    HEADERS += \
        cppquickfix_test_utils.h
    SOURCES += \
        cppdoxygen_test.cpp \
        cppincludehierarchy_test.cpp \
        cppquickfix_test.cpp \
        cppquickfix_test_utils.cpp \
        fileandtokenactions_test.cpp \
        followsymbol_switchmethoddecldef_test.cpp
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

