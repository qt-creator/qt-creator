include(../../qtcreatorplugin.pri)

DEFINES += \
    PYTHONEDITOR_LIBRARY

RESOURCES += \
    pythoneditorplugin.qrc

HEADERS += \
    pythoneditor_global.h \
    pythoneditorplugin.h \
    pythoneditorfactory.h \
    pythoneditor.h \
    pythoneditorwidget.h \
    pythoneditorconstants.h \
    wizard/pythonfilewizard.h \
    wizard/pythonclasswizard.h \
    wizard/pythonclassnamepage.h \
    wizard/pythonclasswizarddialog.h \
    wizard/pythonsourcegenerator.h \
    tools/pythonhighlighter.h \
    tools/pythonindenter.h \
    tools/lexical/pythonformattoken.h \
    tools/lexical/pythonscanner.h \
    tools/lexical/sourcecodestream.h

SOURCES += \
    pythoneditorplugin.cpp \
    pythoneditorfactory.cpp \
    pythoneditor.cpp \
    pythoneditorwidget.cpp \
    wizard/pythonfilewizard.cpp \
    wizard/pythonclasswizarddialog.cpp \
    wizard/pythonclasswizard.cpp \
    wizard/pythonclassnamepage.cpp \
    wizard/pythonsourcegenerator.cpp \
    tools/pythonhighlighter.cpp \
    tools/pythonindenter.cpp \
    tools/lexical/pythonscanner.cpp
