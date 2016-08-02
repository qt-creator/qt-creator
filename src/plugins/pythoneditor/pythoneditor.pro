include(../../qtcreatorplugin.pri)

DEFINES += \
    PYTHONEDITOR_LIBRARY

RESOURCES += \
    pythoneditorplugin.qrc

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
