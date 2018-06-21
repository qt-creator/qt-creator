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
    qmljscomponentfromobjectdef.h \
    qmljsoutline.h \
    qmloutlinemodel.h \
    qmltaskmanager.h \
    qmljsoutlinetreeview.h \
    qmljseditingsettingspage.h \
    quicktoolbar.h \
    qmljscomponentnamedialog.h \
    qmljsfindreferences.h \
    qmljsautocompleter.h \
    qmljsreuse.h \
    qmljsquickfixassist.h \
    qmljscompletionassist.h \
    qmljsquickfix.h \
    qmljssemanticinfoupdater.h \
    qmljssemantichighlighter.h \
    qmljswrapinloader.h \
    qmljseditordocument.h \
    qmljseditordocument_p.h \
    qmljstextmark.h

SOURCES += \
    qmljseditor.cpp \
    qmljseditorplugin.cpp \
    qmlexpressionundercursor.cpp \
    qmljshighlighter.cpp \
    qmljshoverhandler.cpp \
    qmljscomponentfromobjectdef.cpp \
    qmljsoutline.cpp \
    qmloutlinemodel.cpp \
    qmltaskmanager.cpp \
    qmljsquickfixes.cpp \
    qmljsoutlinetreeview.cpp \
    qmljseditingsettingspage.cpp \
    quicktoolbar.cpp \
    qmljscomponentnamedialog.cpp \
    qmljsfindreferences.cpp \
    qmljsautocompleter.cpp \
    qmljsreuse.cpp \
    qmljsquickfixassist.cpp \
    qmljscompletionassist.cpp \
    qmljsquickfix.cpp \
    qmljssemanticinfoupdater.cpp \
    qmljssemantichighlighter.cpp \
    qmljswrapinloader.cpp \
    qmljseditordocument.cpp \
    qmljstextmark.cpp

FORMS += \
    qmljseditingsettingspage.ui \
    qmljscomponentnamedialog.ui
