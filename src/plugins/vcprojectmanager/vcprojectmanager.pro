include(../../qtcreatorplugin.pri)

HEADERS = vcprojectmanagerplugin.h \
    vcprojectreader.h \
    vcprojectnodes.h \
    vcprojectmanagerconstants.h \
    vcprojectmanager_global.h \
    vcprojectmanager.h \
    vcprojectfile.h \
    vcproject.h \
    vcprojectbuildconfiguration.h \
    vcmakestep.h \
    vcprojectbuildoptionspage.h \
    msbuildparser.h
SOURCES = vcprojectmanagerplugin.cpp \
    vcprojectreader.cpp \
    vcprojectnodes.cpp \
    vcprojectmanager.cpp \
    vcprojectfile.cpp \
    vcproject.cpp \
    vcprojectbuildconfiguration.cpp \
    vcmakestep.cpp \
    vcprojectbuildoptionspage.cpp \
    msbuildparser.cpp

OTHER_FILES += \
    VcProject.mimetypes.xml

RESOURCES += \
    vcproject.qrc
