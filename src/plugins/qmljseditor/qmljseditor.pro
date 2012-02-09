TEMPLATE = lib
TARGET = QmlJSEditor
include(../../qtcreatorplugin.pri)
include(qmljseditor_dependencies.pri)
QT += script

DEFINES += \
    QMLJSEDITOR_LIBRARY \
    QT_CREATOR

HEADERS += \
    qmljseditor.h \
    qmljseditor_global.h \
    qmljseditoractionhandler.h \
    qmljseditorconstants.h \
    qmljseditorfactory.h \
    qmljseditorplugin.h \
    qmlexpressionundercursor.h \
    qmlfilewizard.h \
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
    qmljseditoreditable.h \
    qmljsautocompleter.h \
    jsfilewizard.h \
    qmljssnippetprovider.h \
    qmljsreuse.h \
    qmljsquickfixassist.h \
    qmljscompletionassist.h \
    qmljsquickfix.h \
    qmljssemanticinfoupdater.h \
    qmljssemantichighlighter.h \
    qmljswrapinloader.h

SOURCES += \
    qmljseditor.cpp \
    qmljseditoractionhandler.cpp \
    qmljseditorfactory.cpp \
    qmljseditorplugin.cpp \
    qmlexpressionundercursor.cpp \
    qmlfilewizard.cpp \
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
    qmljseditoreditable.cpp \
    qmljsautocompleter.cpp \
    jsfilewizard.cpp \
    qmljssnippetprovider.cpp \
    qmljsreuse.cpp \
    qmljsquickfixassist.cpp \
    qmljscompletionassist.cpp \
    qmljsquickfix.cpp \
    qmljssemanticinfoupdater.cpp \
    qmljssemantichighlighter.cpp \
    qmljswrapinloader.cpp

RESOURCES += qmljseditor.qrc
OTHER_FILES += QmlJSEditor.mimetypes.xml

FORMS += \
    quicktoolbarsettingspage.ui \
    qmljscomponentnamedialog.ui
