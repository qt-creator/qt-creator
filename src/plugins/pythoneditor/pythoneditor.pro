include(../../qtcreatorplugin.pri)

DEFINES += \
    PYTHONEDITOR_LIBRARY

RESOURCES += \
    pythoneditorplugin.qrc

HEADERS += \
    pythoneditorplugin.h \
    pythoneditor.h \
    pythoneditorconstants.h \
    tools/pythonhighlighter.h \
    tools/pythonindenter.h \
    tools/lexical/pythonformattoken.h \
    tools/lexical/pythonscanner.h \
    tools/lexical/sourcecodestream.h

SOURCES += \
    pythoneditorplugin.cpp \
    pythoneditor.cpp \
    tools/pythonhighlighter.cpp \
    tools/pythonindenter.cpp \
    tools/lexical/pythonscanner.cpp
