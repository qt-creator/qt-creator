DEFINES += CPLUSPLUS_BUILD_LIB
INCLUDEPATH += $$PWD

include(../../shared/cplusplus/cplusplus.pri)

contains(QT, gui) {
HEADERS += \
    $$PWD/Icons.h \
    $$PWD/ExpressionUnderCursor.h \
    $$PWD/TokenUnderCursor.h \
    $$PWD/BackwardsScanner.h \
    $$PWD/MatchingText.h \
    $$PWD/OverviewModel.h

SOURCES += \
    $$PWD/Icons.cpp \
    $$PWD/ExpressionUnderCursor.cpp \
    $$PWD/TokenUnderCursor.cpp \
    $$PWD/BackwardsScanner.cpp \
    $$PWD/MatchingText.cpp \
    $$PWD/OverviewModel.cpp
}

HEADERS += \
    $$PWD/SimpleLexer.h \
    $$PWD/CppDocument.h \
    $$PWD/Overview.h \
    $$PWD/NamePrettyPrinter.h \
    $$PWD/TypeOfExpression.h \
    $$PWD/TypePrettyPrinter.h \
    $$PWD/ResolveExpression.h \
    $$PWD/LookupContext.h \
    $$PWD/CppBindings.h \
    $$PWD/ASTParent.h \
    $$PWD/GenTemplateInstance.h \
    $$PWD/CheckUndefinedSymbols.h \
    $$PWD/PreprocessorClient.h \
    $$PWD/PreprocessorEnvironment.h \
    $$PWD/Macro.h \
    $$PWD/FastPreprocessor.h \
    $$PWD/pp.h \
    $$PWD/pp-cctype.h \
    $$PWD/pp-engine.h \
    $$PWD/pp-macro-expander.h \
    $$PWD/pp-scanner.h

SOURCES += \
    $$PWD/SimpleLexer.cpp \
    $$PWD/CppDocument.cpp \
    $$PWD/Overview.cpp \
    $$PWD/NamePrettyPrinter.cpp \
    $$PWD/TypeOfExpression.cpp \
    $$PWD/TypePrettyPrinter.cpp \
    $$PWD/ResolveExpression.cpp \
    $$PWD/LookupContext.cpp \
    $$PWD/CppBindings.cpp \
    $$PWD/ASTParent.cpp \
    $$PWD/GenTemplateInstance.cpp \
    $$PWD/CheckUndefinedSymbols.cpp \
    $$PWD/PreprocessorClient.cpp \
    $$PWD/PreprocessorEnvironment.cpp \
    $$PWD/FastPreprocessor.cpp \
    $$PWD/Macro.cpp \
    $$PWD/pp-engine.cpp \
    $$PWD/pp-macro-expander.cpp \
    $$PWD/pp-scanner.cpp

RESOURCES += $$PWD/cplusplus.qrc
