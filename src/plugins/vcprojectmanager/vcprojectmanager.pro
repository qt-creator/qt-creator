include(../../qtcreatorplugin.pri)

HEADERS = vcprojectmanagerplugin.h \
    vcprojectreader.h \
    vcprojectnodes.h \
    vcprojectmanagerconstants.h \
    vcprojectmanager_global.h \
    vcprojectmanager.h \
    vcprojectfile.h \
    vcproject.h
SOURCES = vcprojectmanagerplugin.cpp \
    vcprojectreader.cpp \
    vcprojectnodes.cpp \
    vcprojectmanager.cpp \
    vcprojectfile.cpp \
    vcproject.cpp

OTHER_FILES += \
    VcProject.mimetypes.xml

RESOURCES += \
    vcproject.qrc
