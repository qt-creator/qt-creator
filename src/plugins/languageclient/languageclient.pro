include(../../qtcreatorplugin.pri)

DEFINES += LANGUAGECLIENT_LIBRARY

HEADERS += \
    client.h \
    documentsymbolcache.h \
    dynamiccapabilities.h \
    languageclient_global.h \
    languageclientcompletionassist.h \
    languageclientfunctionhint.h \
    languageclienthoverhandler.h \
    languageclientinterface.h \
    languageclientmanager.h \
    languageclientoutline.h \
    languageclientplugin.h \
    languageclientquickfix.h \
    languageclientsettings.h \
    languageclientutils.h \
    locatorfilter.h \
    semantichighlightsupport.h


SOURCES += \
    client.cpp \
    documentsymbolcache.cpp \
    dynamiccapabilities.cpp \
    languageclientcompletionassist.cpp \
    languageclientfunctionhint.cpp \
    languageclienthoverhandler.cpp \
    languageclientinterface.cpp \
    languageclientmanager.cpp \
    languageclientoutline.cpp \
    languageclientplugin.cpp \
    languageclientquickfix.cpp \
    languageclientsettings.cpp \
    languageclientutils.cpp \
    locatorfilter.cpp \
    semantichighlightsupport.cpp

RESOURCES += \
    languageclient.qrc
