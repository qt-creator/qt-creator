include(../../qtcreatorplugin.pri)

HEADERS += \
    b2buildconfiguration.h \
    b2buildinfo.h \
    b2buildstep.h \
    b2openprojectwizard.h \
    b2outputparser.h \
    b2project.h \
    b2projectfile.h \
    b2projectmanager.h \
    b2projectmanager_global.h \
    b2projectmanagerconstants.h \
    b2projectmanagerplugin.h \
    b2projectnode.h \
    b2utility.h \
    filesselectionwizardpage.h \
    selectablefilesmodel.h \
    external/projectexplorer/clangparser.h \
    external/projectexplorer/gccparser.h \
    external/projectexplorer/ldparser.h

SOURCES += \
    b2buildconfiguration.cpp \
    b2buildinfo.cpp \
    b2buildstep.cpp \
    b2openprojectwizard.cpp \
    b2outputparser.cpp \
    b2project.cpp \
    b2projectfile.cpp \
    b2projectmanager.cpp \
    b2projectmanagerplugin.cpp \
    b2projectnode.cpp \
    b2utility.cpp \
    filesselectionwizardpage.cpp \
    selectablefilesmodel.cpp \
    external/projectexplorer/clangparser.cpp \
    external/projectexplorer/gccparser.cpp \
    external/projectexplorer/ldparser.cpp

RESOURCES += \
    boostbuildproject.qrc

OTHER_FILES += \
    $${QTC_PLUGIN_NAME}.mimetypes.xml \
    $${QTC_PLUGIN_NAME}.pluginspec.in
