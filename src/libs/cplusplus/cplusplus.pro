TEMPLATE = lib

TARGET = CPlusPlus

DEFINES += HAVE_QT CPLUSPLUS_WITH_NAMESPACE CPLUSPLUS_BUILD_LIB
DEFINES += NDEBUG
unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qworkbenchlibrary.pri)
include(../../shared/cplusplus/cplusplus.pri)

HEADERS += \
    SimpleLexer.h \
    ExpressionUnderCursor.h \
    TokenUnderCursor.h \
    CppDocument.h \
    Icons.h \
    Overview.h \
    OverviewModel.h \
    NamePrettyPrinter.h \
    TypeOfExpression.h \
    TypePrettyPrinter.h \
    ResolveExpression.h \
    LookupContext.h \
    PreprocessorClient.h \
    PreprocessorEnvironment.h \
    Macro.h \
    pp.h \
    pp-cctype.h \
    pp-engine.h \
    pp-macro-expander.h \
    pp-scanner.h

SOURCES += \
    SimpleLexer.cpp \
    ExpressionUnderCursor.cpp \
    TokenUnderCursor.cpp \
    CppDocument.cpp \
    Icons.cpp \
    Overview.cpp \
    OverviewModel.cpp \
    NamePrettyPrinter.cpp \
    TypeOfExpression.cpp \
    TypePrettyPrinter.cpp \
    ResolveExpression.cpp \
    LookupContext.cpp \
    PreprocessorClient.cpp \
    PreprocessorEnvironment.cpp \
    Macro.cpp \
    pp-engine.cpp \
    pp-macro-expander.cpp \
    pp-scanner.cpp

RESOURCES += cplusplus.qrc
