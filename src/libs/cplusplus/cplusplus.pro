TEMPLATE = lib

TARGET = CPlusPlus

DEFINES += HAVE_QT CPLUSPLUS_WITH_NAMESPACE CPLUSPLUS_BUILD_LIB
DEFINES += NDEBUG
unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qworkbenchlibrary.pri)
include(../../../shared/cplusplus/cplusplus.pri)

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
    LookupContext.h

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
    LookupContext.cpp

RESOURCES += cplusplus.qrc
