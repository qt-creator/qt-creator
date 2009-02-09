DEFINES += HAVE_QT CPLUSPLUS_WITH_NAMESPACE CPLUSPLUS_BUILD_LIB
INCLUDEPATH += $$PWD

include(../../shared/cplusplus/cplusplus.pri)

HEADERS += \
    $$PWD/SimpleLexer.h \
    $$PWD/ExpressionUnderCursor.h \
    $$PWD/TokenUnderCursor.h \
    $$PWD/CppDocument.h \
    $$PWD/Icons.h \
    $$PWD/Overview.h \
    $$PWD/OverviewModel.h \
    $$PWD/NamePrettyPrinter.h \
    $$PWD/TypeOfExpression.h \
    $$PWD/TypePrettyPrinter.h \
    $$PWD/ResolveExpression.h \
    $$PWD/LookupContext.h \
    $$PWD/PreprocessorClient.h \
    $$PWD/PreprocessorEnvironment.h \
    $$PWD/Macro.h \
    $$PWD/pp.h \
    $$PWD/pp-cctype.h \
    $$PWD/pp-engine.h \
    $$PWD/pp-macro-expander.h \
    $$PWD/pp-scanner.h

SOURCES += \
    $$PWD/SimpleLexer.cpp \
    $$PWD/ExpressionUnderCursor.cpp \
    $$PWD/TokenUnderCursor.cpp \
    $$PWD/CppDocument.cpp \
    $$PWD/Icons.cpp \
    $$PWD/Overview.cpp \
    $$PWD/OverviewModel.cpp \
    $$PWD/NamePrettyPrinter.cpp \
    $$PWD/TypeOfExpression.cpp \
    $$PWD/TypePrettyPrinter.cpp \
    $$PWD/ResolveExpression.cpp \
    $$PWD/LookupContext.cpp \
    $$PWD/PreprocessorClient.cpp \
    $$PWD/PreprocessorEnvironment.cpp \
    $$PWD/Macro.cpp \
    $$PWD/pp-engine.cpp \
    $$PWD/pp-macro-expander.cpp \
    $$PWD/pp-scanner.cpp

RESOURCES += $$PWD/cplusplus.qrc
