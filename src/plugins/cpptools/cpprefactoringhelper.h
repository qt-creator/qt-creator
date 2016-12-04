#ifndef CPPREFACTPRINGHELPER_H
#define CPPREFACTPRINGHELPER_H

#include "cplusplus/FindUsages.h"
#include "cplusplus/Symbol.h"
#include <cpptools/cppmodelmanager.h>
#include "cpptools/cpprefactoringchanges.h"
#include "cppworkingcopy.h"
#include <projectexplorer/projectnodes.h>
#include "qstringlist.h"
#include "utils/changeset.h"
#include "cpptools_global.h"



class CPPTOOLS_EXPORT CppRefactoringHelper
{
public:
    static QString buildFromNamespaceElements(const QStringList &namespaceElements);
    static bool isHeaderFile(const QString& filename);
    typedef bool (SymbolFilter)(const QChar &);
    static bool isCharOfClassCompletionSymbolForward(const QChar& qChar);
    static bool isCharOfClassCompletionSymbolBackward(const QChar& qChar);
    static Utils::ChangeSet::Range getSymbolRange(CppTools::CppRefactoringFilePtr &file, int initialPosition,
                                                  SymbolFilter* symbolFilterBackward, SymbolFilter* symbolFilterForward);
    static QString getSymbolAtPosition(CppTools::CppRefactoringFilePtr &file, int initialPosition, SymbolFilter* symbolFilterBackward,
                                       SymbolFilter* symbolFilterForward, int& symbolPosition);

    static void getNamespaceAndClassElementsFromSymbol(CPlusPlus::Symbol* symbol, QStringList& vNamespaceElements, QStringList& vClassElements, QString& otherElements);
    static void getNamespaceAndClassElementsFromScope(CPlusPlus::Scope* scope, QStringList& vNamespaceElements,
                                                      QStringList& vClassAsNamespaceElements);
    static void getNamespaceElementsFromClass(CPlusPlus::Class* clazz, QStringList& vNamespaceElements);
    static void reduceNamespace(const QStringList& vNamespaceUsed, const QStringList& vNamespaceUsing, QStringList& vDiffNamespace);
    static void getNamespaceAliasesInFile(CppTools::CppRefactoringFilePtr &cppFile, QMap<QString, QString> &namespaceAliases, QMap<QString, int> &namespaceAliasesLine);
    static void getNamespaceAndClass(const QString& fullySpecifiedClassName, QStringList& vNamespaceElements, QStringList& vClassAsNamespaceElements, QString& className);
};

#endif
