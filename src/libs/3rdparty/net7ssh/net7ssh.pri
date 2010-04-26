include(net7ssh_dependencies.pri)
INCLUDEPATH *= $$PWD/src
LIBS *= -l$$qtLibraryTarget(Net7ssh)
