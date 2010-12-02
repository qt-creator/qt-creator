TEMPLATE = lib
TARGET = QmlJSEditor
include(../../qtcreatorplugin.pri)
include(qmljseditor_dependencies.pri)

DEFINES += \
    QMLJSEDITOR_LIBRARY \
    QT_CREATOR

HEADERS += \
    qmljscodecompletion.h \
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
    qmljsquickfix.h \
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
    qmljssemantichighlighter.h \
    qmljsindenter.h \
    qmljsautocompleter.h \
    jsfilewizard.h \
    qmljssnippetprovider.h

SOURCES += \
    qmljscodecompletion.cpp \
    qmljseditor.cpp \
    qmljseditoractionhandler.cpp \
    qmljseditorfactory.cpp \
    qmljseditorplugin.cpp \
    qmlexpressionundercursor.cpp \
    qmlfilewizard.cpp \
    qmljshighlighter.cpp \
    qmljshoverhandler.cpp \
    qmljspreviewrunner.cpp \
    qmljsquickfix.cpp \
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
    qmljssemantichighlighter.cpp \
    qmljsindenter.cpp \
    qmljsautocompleter.cpp \
    jsfilewizard.cpp \
    qmljssnippetprovider.cpp

RESOURCES += qmljseditor.qrc
OTHER_FILES += QmlJSEditor.mimetypes.xml

FORMS += \
    quicktoolbarsettingspage.ui \
    qmljscomponentnamedialog.ui
