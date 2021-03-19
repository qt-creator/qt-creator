isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE = $$(QTC_SOURCE)
isEmpty(IDE_SOURCE_TREE): error("You need to set the environment variable QTC_SOURCE to point to the directory where the Qt Creator sources are")

isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE): error("You need to set the environment variable QTC_BUILD to point to the directory where Qt Creator was built")

include($$IDE_SOURCE_TREE/src/qtcreatorplugin.pri)
SOURCES += \
    fossilclient.cpp \
    fossilplugin.cpp \
    fossilsettings.cpp \
    commiteditor.cpp \
    fossilcommitwidget.cpp \
    fossileditor.cpp \
    annotationhighlighter.cpp \
    pullorpushdialog.cpp \
    branchinfo.cpp \
    configuredialog.cpp \
    revisioninfo.cpp \
    wizard/fossiljsextension.cpp
HEADERS += \
    fossilclient.h \
    constants.h \
    fossilplugin.h \
    fossilsettings.h \
    commiteditor.h \
    fossilcommitwidget.h \
    fossileditor.h \
    annotationhighlighter.h \
    pullorpushdialog.h \
    branchinfo.h \
    configuredialog.h \
    revisioninfo.h \
    wizard/fossiljsextension.h
FORMS += \
    revertdialog.ui \
    fossilcommitpanel.ui \
    pullorpushdialog.ui \
    configuredialog.ui
RESOURCES += fossil.qrc
