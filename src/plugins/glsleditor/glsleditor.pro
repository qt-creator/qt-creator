include(../../qtcreatorplugin.pri)

DEFINES += \
    GLSLEDITOR_LIBRARY

HEADERS += \
glsleditor.h \
glsleditorconstants.h \
glsleditorplugin.h \
glslhighlighter.h \
glslautocompleter.h \
glslindenter.h \
glslhoverhandler.h \
glslcompletionassist.h

SOURCES += \
glsleditor.cpp \
glsleditorplugin.cpp \
glslhighlighter.cpp \
glslautocompleter.cpp \
glslindenter.cpp \
glslhoverhandler.cpp \
glslcompletionassist.cpp

RESOURCES += glsleditor.qrc
