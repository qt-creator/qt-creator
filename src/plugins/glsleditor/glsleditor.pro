include(../../qtcreatorplugin.pri)

DEFINES += \
    GLSLEDITOR_LIBRARY

HEADERS += \
glsleditor.h \
glsleditor_global.h \
glsleditorconstants.h \
glsleditoreditable.h \
glsleditorfactory.h \
glsleditorplugin.h \
glslfilewizard.h \
glslhighlighter.h \
glslautocompleter.h \
glslindenter.h \
glslhoverhandler.h \
    glslcompletionassist.h \
    reuse.h

SOURCES += \
glsleditor.cpp \
glsleditoreditable.cpp \
glsleditorfactory.cpp \
glsleditorplugin.cpp \
glslfilewizard.cpp \
glslhighlighter.cpp \
glslautocompleter.cpp \
glslindenter.cpp \
glslhoverhandler.cpp \
    glslcompletionassist.cpp \
    reuse.cpp

RESOURCES += glsleditor.qrc
