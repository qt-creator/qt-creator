contains(CONFIG, dll) {
    DEFINES += CPLUSPLUS_BUILD_LIB
} else {
    DEFINES += CPLUSPLUS_BUILD_STATIC_LIB
}

INCLUDEPATH += $$PWD

include(../3rdparty/cplusplus/cplusplus.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

#DEFINES += DEBUG_INCLUDE_GUARD_TRACKING

contains(QT, gui) {
HEADERS += \
    $$PWD/Icons.h \
    $$PWD/ExpressionUnderCursor.h \
    $$PWD/BackwardsScanner.h \
    $$PWD/MatchingText.h \
    $$PWD/OverviewModel.h

SOURCES += \
    $$PWD/Icons.cpp \
    $$PWD/ExpressionUnderCursor.cpp \
    $$PWD/BackwardsScanner.cpp \
    $$PWD/MatchingText.cpp \
    $$PWD/OverviewModel.cpp
}

HEADERS += \
    $$PWD/SimpleLexer.h \
    $$PWD/CppDocument.h \
    $$PWD/CppRewriter.h \
    $$PWD/Overview.h \
    $$PWD/NamePrettyPrinter.h \
    $$PWD/TypeOfExpression.h \
    $$PWD/TypePrettyPrinter.h \
    $$PWD/ResolveExpression.h \
    $$PWD/LookupItem.h \
    $$PWD/AlreadyConsideredClassContainer.h \
    $$PWD/LookupContext.h \
    $$PWD/ASTParent.h \
    $$PWD/ASTPath.h \
    $$PWD/SnapshotSymbolVisitor.h \
    $$PWD/SymbolNameVisitor.h \
    $$PWD/DeprecatedGenTemplateInstance.h \
    $$PWD/FindUsages.h \
    $$PWD/DependencyTable.h \
    $$PWD/PreprocessorClient.h \
    $$PWD/PreprocessorEnvironment.h \
    $$PWD/Macro.h \
    $$PWD/FastPreprocessor.h \
    $$PWD/pp.h \
    $$PWD/pp-cctype.h \
    $$PWD/pp-engine.h \
    $$PWD/pp-scanner.h \
    $$PWD/findcdbbreakpoint.h \
    $$PWD/PPToken.h \
    $$PWD/Dumpers.h

SOURCES += \
    $$PWD/SimpleLexer.cpp \
    $$PWD/CppDocument.cpp \
    $$PWD/CppRewriter.cpp \
    $$PWD/Overview.cpp \
    $$PWD/NamePrettyPrinter.cpp \
    $$PWD/TypeOfExpression.cpp \
    $$PWD/TypePrettyPrinter.cpp \
    $$PWD/ResolveExpression.cpp \
    $$PWD/LookupItem.cpp \
    $$PWD/LookupContext.cpp \
    $$PWD/ASTParent.cpp \
    $$PWD/ASTPath.cpp \
    $$PWD/SnapshotSymbolVisitor.cpp \
    $$PWD/SymbolNameVisitor.cpp \
    $$PWD/DeprecatedGenTemplateInstance.cpp \
    $$PWD/FindUsages.cpp \
    $$PWD/DependencyTable.cpp \
    $$PWD/PreprocessorClient.cpp \
    $$PWD/PreprocessorEnvironment.cpp \
    $$PWD/FastPreprocessor.cpp \
    $$PWD/Macro.cpp \
    $$PWD/pp-engine.cpp \
    $$PWD/pp-scanner.cpp \
    $$PWD/findcdbbreakpoint.cpp \
    $$PWD/PPToken.cpp \
    $$PWD/Dumpers.cpp

RESOURCES += $$PWD/cplusplus.qrc
