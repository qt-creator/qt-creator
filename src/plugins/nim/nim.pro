include(../../qtcreatorplugin.pri)

DEFINES += \
    NIM_LIBRARY

RESOURCES += \
    nim.qrc

INCLUDEPATH += $$PWD

HEADERS += \
    nimplugin.h \
    nimconstants.h \
    editor/nimhighlighter.h \
    editor/nimindenter.h \
    tools/nimlexer.h \
    tools/sourcecodestream.h \
    project/nimproject.h \
    project/nimprojectnode.h \
    project/nimbuildconfiguration.h \
    project/nimbuildconfigurationfactory.h \
    project/nimcompilerbuildstep.h \
    project/nimcompilerbuildstepconfigwidget.h \
    project/nimcompilercleanstep.h \
    project/nimcompilercleanstepconfigwidget.h \
    project/nimrunconfigurationfactory.h \
    project/nimrunconfiguration.h \
    project/nimrunconfigurationwidget.h \
    project/nimcompilerbuildstepfactory.h \
    project/nimcompilercleanstepfactory.h \
    project/nimbuildconfigurationwidget.h \
    editor/nimeditorfactory.h \
    settings/nimcodestylesettingspage.h \
    settings/nimcodestylepreferencesfactory.h \
    settings/nimsettings.h \
    settings/nimcodestylepreferenceswidget.h \
    project/nimtoolchain.h \
    project/nimtoolchainfactory.h

SOURCES += \
    nimplugin.cpp \
    editor/nimhighlighter.cpp \
    editor/nimindenter.cpp \
    tools/nimlexer.cpp \
    project/nimproject.cpp \
    project/nimprojectnode.cpp \
    project/nimbuildconfiguration.cpp \
    project/nimbuildconfigurationfactory.cpp \
    project/nimcompilerbuildstep.cpp \
    project/nimcompilerbuildstepconfigwidget.cpp \
    project/nimcompilercleanstep.cpp \
    project/nimcompilercleanstepconfigwidget.cpp \
    project/nimrunconfigurationfactory.cpp \
    project/nimrunconfiguration.cpp \
    project/nimrunconfigurationwidget.cpp \
    project/nimcompilerbuildstepfactory.cpp \
    project/nimcompilercleanstepfactory.cpp \
    project/nimbuildconfigurationwidget.cpp \
    editor/nimeditorfactory.cpp \
    settings/nimcodestylesettingspage.cpp \
    settings/nimcodestylepreferencesfactory.cpp \
    settings/nimsettings.cpp \
    settings/nimcodestylepreferenceswidget.cpp \
    project/nimtoolchain.cpp \
    project/nimtoolchainfactory.cpp

FORMS += \
    project/nimcompilerbuildstepconfigwidget.ui \
    project/nimcompilercleanstepconfigwidget.ui \
    settings/nimcodestylepreferenceswidget.ui
