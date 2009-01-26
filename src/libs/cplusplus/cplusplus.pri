INCLUDEPATH += $$PWD/../../shared/cplusplus
DEFINES += HAVE_QT CPLUSPLUS_WITH_NAMESPACE
LIBS *= -l$$qtLibraryTarget(CPlusPlus)
