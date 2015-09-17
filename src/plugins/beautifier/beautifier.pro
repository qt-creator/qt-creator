include(../../qtcreatorplugin.pri)

HEADERS += \
    abstractsettings.h \
    beautifierabstracttool.h \
    beautifierconstants.h \
    beautifierplugin.h \
    command.h \
    configurationdialog.h \
    configurationeditor.h \
    configurationpanel.h \
    generaloptionspage.h \
    generalsettings.h \
    artisticstyle/artisticstyleconstants.h \
    artisticstyle/artisticstyle.h \
    artisticstyle/artisticstyleoptionspage.h \
    artisticstyle/artisticstylesettings.h \
    clangformat/clangformatconstants.h \
    clangformat/clangformat.h \
    clangformat/clangformatoptionspage.h \
    clangformat/clangformatsettings.h \
    uncrustify/uncrustifyconstants.h \
    uncrustify/uncrustify.h \
    uncrustify/uncrustifyoptionspage.h \
    uncrustify/uncrustifysettings.h

SOURCES += \
    abstractsettings.cpp \
    beautifierplugin.cpp \
    command.cpp \
    configurationdialog.cpp \
    configurationeditor.cpp \
    configurationpanel.cpp \
    generaloptionspage.cpp \
    generalsettings.cpp \
    artisticstyle/artisticstyle.cpp \
    artisticstyle/artisticstyleoptionspage.cpp \
    artisticstyle/artisticstylesettings.cpp \
    clangformat/clangformat.cpp \
    clangformat/clangformatoptionspage.cpp \
    clangformat/clangformatsettings.cpp \
    uncrustify/uncrustify.cpp \
    uncrustify/uncrustifyoptionspage.cpp \
    uncrustify/uncrustifysettings.cpp

FORMS += \
    configurationdialog.ui \
    configurationpanel.ui \
    generaloptionspage.ui \
    artisticstyle/artisticstyleoptionspage.ui \
    clangformat/clangformatoptionspage.ui \
    uncrustify/uncrustifyoptionspage.ui \

RESOURCES += \
    beautifier.qrc

