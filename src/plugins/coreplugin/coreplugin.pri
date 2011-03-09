include(coreplugin_dependencies.pri)
LIBS *= -l$$qtLibraryName(Core)
# for ide_version.h
INCLUDEPATH *= $$IDE_BUILD_TREE/src/plugins/coreplugin
