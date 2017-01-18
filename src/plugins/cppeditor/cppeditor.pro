DEFINES += CPPEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS += \
    cppautocompleter.h \
    cppcanonicalsymbol.h \
    cppcodemodelinspectordialog.h \
    cppdocumentationcommenthelper.h \
    cppeditor.h \
    cppeditor_global.h \
    cppeditordocument.h \
    cppeditorconstants.h \
    cppeditorenums.h \
    cppeditorplugin.h \
    cppelementevaluator.h \
    cppfollowsymbolundercursor.h \
    cppfunctiondecldeflink.h \
    cpphighlighter.h \
    cpphoverhandler.h \
    cppparsecontext.h \
    cppincludehierarchy.h \
    cppinsertvirtualmethods.h \
    cpplocalrenaming.h \
    cppminimizableinfobars.h \
    cppoutline.h \
    cpppreprocessordialog.h \
    cppquickfix.h \
    cppquickfixassistant.h \
    cppquickfixes.h \
    cppsnippetprovider.h \
    cpptypehierarchy.h \
    cppuseselectionsupdater.h \
    cppvirtualfunctionassistprovider.h \
    cppvirtualfunctionproposalitem.h \
    resourcepreviewhoverhandler.h

SOURCES += \
    cppautocompleter.cpp \
    cppcanonicalsymbol.cpp \
    cppcodemodelinspectordialog.cpp \
    cppdocumentationcommenthelper.cpp \
    cppeditor.cpp \
    cppeditordocument.cpp \
    cppeditorplugin.cpp \
    cppelementevaluator.cpp \
    cppfollowsymbolundercursor.cpp \
    cppfunctiondecldeflink.cpp \
    cpphighlighter.cpp \
    cpphoverhandler.cpp \
    cppparsecontext.cpp \
    cppincludehierarchy.cpp \
    cppinsertvirtualmethods.cpp \
    cpplocalrenaming.cpp \
    cppminimizableinfobars.cpp \
    cppoutline.cpp \
    cpppreprocessordialog.cpp \
    cppquickfix.cpp \
    cppquickfixassistant.cpp \
    cppquickfixes.cpp \
    cppsnippetprovider.cpp \
    cpptypehierarchy.cpp \
    cppuseselectionsupdater.cpp \
    cppvirtualfunctionassistprovider.cpp \
    cppvirtualfunctionproposalitem.cpp \
    resourcepreviewhoverhandler.cpp

FORMS += \
    cpppreprocessordialog.ui \
    cppcodemodelinspectordialog.ui

RESOURCES += \
    cppeditor.qrc

equals(TEST, 1) {
    HEADERS += \
        cppeditortestcase.h \
        cppdoxygen_test.h \
        cppquickfix_test.h
    SOURCES += \
        cppdoxygen_test.cpp \
        cppeditortestcase.cpp \
        cppincludehierarchy_test.cpp \
        cppquickfix_test.cpp \
        cppuseselections_test.cpp \
        fileandtokenactions_test.cpp \
        followsymbol_switchmethoddecldef_test.cpp
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
