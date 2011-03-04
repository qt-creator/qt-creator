include(analyzerbase_dependencies.pri)

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
LIBS *= -l$$qtLibraryName(AnalyzerBase)
