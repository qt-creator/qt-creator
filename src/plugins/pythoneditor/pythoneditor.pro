include(../../qtcreatorplugin.pri)

DEFINES += \
    PYTHONEDITOR_LIBRARY

HEADERS += \
    pythoneditorplugin.h \
    pythoneditor.h \
    pythoneditorconstants.h \
    pythonhighlighter.h \
    pythonindenter.h \
    pythonformattoken.h \
    pythonscanner.h \

SOURCES += \
    pythoneditorplugin.cpp \
    pythoneditor.cpp \
    pythonhighlighter.cpp \
    pythonindenter.cpp \
    pythonscanner.cpp
