QT += xml xmlpatterns

HEADERS = vcprojectmanagerplugin.h \
    vcprojectmanagerconstants.h \
    vcprojectmanager_global.h \
    vcprojectmanager.h \
    vcprojectfile.h \
    vcproject.h \
    vcprojectbuildconfiguration.h \
    vcmakestep.h \
    msbuildoutputparser.h \
    vcprojectkitinformation.h \
    msbuildversionmanager.h \
    vcprojectbuildoptionspage.h \
    vcschemamanager.h \
    menuhandler.h

SOURCES = vcprojectmanagerplugin.cpp \
    vcprojectmanager.cpp \
    vcprojectfile.cpp \
    vcproject.cpp \
    vcprojectbuildconfiguration.cpp \
    vcmakestep.cpp \
    msbuildoutputparser.cpp \
    vcprojectkitinformation.cpp \
    msbuildversionmanager.cpp \
    vcprojectbuildoptionspage.cpp \
    vcschemamanager.cpp \
    menuhandler.cpp

OTHER_FILES += \
    VcProject.mimetypes.xml

RESOURCES += \
    vcproject.qrc

include($$PWD/vcprojectmodel/vcprojectmodel.pri)
include($$PWD/widgets/widgets.pri)
include(../../qtcreatorplugin.pri)
include(interfaces/interfaces.pri)
