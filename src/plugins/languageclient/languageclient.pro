include(../../qtcreatorplugin.pri)

DEFINES += LANGUAGECLIENT_LIBRARY

HEADERS += \
    client.h \
    diagnosticmanager.h \
    documentsymbolcache.h \
    dynamiccapabilities.h \
    languageclient_global.h \
    languageclientcompletionassist.h \
    languageclientformatter.h \
    languageclientfunctionhint.h \
    languageclienthoverhandler.h \
    languageclientinterface.h \
    languageclientmanager.h \
    languageclientoutline.h \
    languageclientplugin.h \
    languageclientquickfix.h \
    languageclientsettings.h \
    languageclientsymbolsupport.h \
    languageclientutils.h \
    locatorfilter.h \
    lspinspector.h \
    semantichighlightsupport.h


SOURCES += \
    client.cpp \
    diagnosticmanager.cpp \
    documentsymbolcache.cpp \
    dynamiccapabilities.cpp \
    languageclientcompletionassist.cpp \
    languageclientformatter.cpp \
    languageclientfunctionhint.cpp \
    languageclienthoverhandler.cpp \
    languageclientinterface.cpp \
    languageclientmanager.cpp \
    languageclientoutline.cpp \
    languageclientplugin.cpp \
    languageclientquickfix.cpp \
    languageclientsettings.cpp \
    languageclientsymbolsupport.cpp \
    languageclientutils.cpp \
    locatorfilter.cpp \
    lspinspector.cpp \
    semantichighlightsupport.cpp

RESOURCES += \
    languageclient.qrc
