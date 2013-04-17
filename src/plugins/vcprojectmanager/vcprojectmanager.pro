include(../../qtcreatorplugin.pri)

QT += xml xmlpatterns

HEADERS = vcprojectmanagerplugin.h \
    vcprojectmanagerconstants.h \
    vcprojectmanager_global.h \
    vcprojectmanager.h \
    vcprojectfile.h \
    vcproject.h \
    vcprojectbuildconfiguration.h \
    vcmakestep.h \
    vcprojectbuildoptionspage.h \
    msbuildoutputparser.h
SOURCES = vcprojectmanagerplugin.cpp \
    vcprojectmanager.cpp \
    vcprojectfile.cpp \
    vcproject.cpp \
    vcprojectbuildconfiguration.cpp \
    vcmakestep.cpp \
    vcprojectbuildoptionspage.cpp \
    msbuildoutputparser.cpp

OTHER_FILES += \
    VcProject.mimetypes.xml

include($$PWD/vcprojectmodel/vcprojectmodel.pri)
include($$PWD/widgets/widgets.pri)

RESOURCES += \
    vcproject.qrc
