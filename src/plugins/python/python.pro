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
    pythonsettings.h \
    pythonutils.h

SOURCES += \
    pythonplugin.cpp \
    pythoneditor.cpp \
    pythonhighlighter.cpp \
    pythonindenter.cpp \
    pythonproject.cpp \
    pythonrunconfiguration.cpp \
    pythonscanner.cpp \
    pythonsettings.cpp \
    pythonutils.cpp

RESOURCES += \
    python.qrc
