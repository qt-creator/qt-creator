include(../../qtcreatorplugin.pri)

DEFINES += \
    PYTHON_LIBRARY

HEADERS += \
    pythonplugin.h \
    pythoneditor.h \
    pythonconstants.h \
    pythonhighlighter.h \
    pythonindenter.h \
    pythonformattoken.h \
    pythonproject.h \
    pythonrunconfiguration.h \
    pythonscanner.h \
    pythonsettings.h

SOURCES += \
    pythonplugin.cpp \
    pythoneditor.cpp \
    pythonhighlighter.cpp \
    pythonindenter.cpp \
    pythonproject.cpp \
    pythonrunconfiguration.cpp \
    pythonscanner.cpp \
    pythonsettings.cpp

RESOURCES += \
    python.qrc
