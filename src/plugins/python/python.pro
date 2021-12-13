include(../../qtcreatorplugin.pri)

DEFINES += \
    PYTHON_LIBRARY

HEADERS += \
    pythonconstants.h \
    pythoneditor.h \
    pythonformattoken.h \
    pythonhighlighter.h \
    pythonindenter.h \
    pythonlanguageclient.h \
    pythonplugin.h \
    pythonproject.h \
    pythonrunconfiguration.h \
    pythonscanner.h \
    pythonsettings.h \
    pythonutils.h \

SOURCES += \
    pythoneditor.cpp \
    pythonhighlighter.cpp \
    pythonindenter.cpp \
    pythonlanguageclient.cpp \
    pythonplugin.cpp \
    pythonproject.cpp \
    pythonrunconfiguration.cpp \
    pythonscanner.cpp \
    pythonsettings.cpp \
    pythonutils.cpp \

RESOURCES += \
    python.qrc
