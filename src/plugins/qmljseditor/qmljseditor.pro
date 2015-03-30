include(../../qtcreatorplugin.pri)

DEFINES += \
    QMLJSEDITOR_LIBRARY

HEADERS += \
    qmljseditor.h \
    qmljseditor_global.h \
    qmljseditorconstants.h \
    qmljseditorplugin.h \
    qmlexpressionundercursor.h \
    qmljshighlighter.h \
    qmljshoverhandler.h \
    qmljspreviewrunner.h \
    qmljscomponentfromobjectdef.h \
    qmljsoutline.h \
    qmloutlinemodel.h \
    qmltaskmanager.h \
    qmljsoutlinetreeview.h \
    quicktoolbarsettingspage.h \
    quicktoolbar.h \
    qmljscomponentnamedialog.h \
    qmljsfindreferences.h \
    qmljsautocompleter.h \
    qmljssnippetprovider.h \
    qmljsreuse.h \
    qmljsquickfixassist.h \
    qmljscompletionassist.h \
    qmljsquickfix.h \
    qmljssemanticinfoupdater.h \
    qmljssemantichighlighter.h \
    qmljswrapinloader.h \
    qmljseditordocument.h \
    qmljseditordocument_p.h

SOURCES += \
    qmljseditor.cpp \
    qmljseditorplugin.cpp \
    qmlexpressionundercursor.cpp \
    qmljshighlighter.cpp \
    qmljshoverhandler.cpp \
    qmljspreviewrunner.cpp \
    qmljscomponentfromobjectdef.cpp \
    qmljsoutline.cpp \
    qmloutlinemodel.cpp \
    qmltaskmanager.cpp \
    qmljsquickfixes.cpp \
    qmljsoutlinetreeview.cpp \
    quicktoolbarsettingspage.cpp \
    quicktoolbar.cpp \
    qmljscomponentnamedialog.cpp \
    qmljsfindreferences.cpp \
    qmljsautocompleter.cpp \
    qmljssnippetprovider.cpp \
    qmljsreuse.cpp \
    qmljsquickfixassist.cpp \
    qmljscompletionassist.cpp \
    qmljsquickfix.cpp \
    qmljssemanticinfoupdater.cpp \
    qmljssemantichighlighter.cpp \
    qmljswrapinloader.cpp \
    qmljseditordocument.cpp

RESOURCES += qmljseditor.qrc

FORMS += \
    quicktoolbarsettingspage.ui \
    qmljscomponentnamedialog.ui
