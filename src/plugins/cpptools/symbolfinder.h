#ifndef SYMBOLFINDER_H
#define SYMBOLFINDER_H

#include "cpptools_global.h"

#include <CppDocument.h>
#include <CPlusPlusForwardDeclarations.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QQueue>
#include <QtCore/QMultiMap>

namespace CppTools {

class CPPTOOLS_EXPORT SymbolFinder
{
public:
    SymbolFinder(const QString &referenceFileName);
    SymbolFinder(const char *referenceFileName, unsigned referenceFileLength);

    CPlusPlus::Symbol *findMatchingDefinition(CPlusPlus::Symbol *symbol,
                                              const CPlusPlus::Snapshot &snapshot,
                                              bool strict = false);

    CPlusPlus::Class *findMatchingClassDeclaration(CPlusPlus::Symbol *declaration,
                                                   const CPlusPlus::Snapshot &snapshot);

    void findMatchingDeclaration(const CPlusPlus::LookupContext &context,
                                 CPlusPlus::Function *functionType,
                                 QList<CPlusPlus::Declaration *> *typeMatch,
                                 QList<CPlusPlus::Declaration *> *argumentCountMatch,
                                 QList<CPlusPlus::Declaration *> *nameMatch);

    QList<CPlusPlus::Declaration *> findMatchingDeclaration(const CPlusPlus::LookupContext &context,
                                                            CPlusPlus::Function *functionType);

private:
    QStringList fileIterationOrder(const CPlusPlus::Snapshot &snapshot);

    void checkCacheConsistency(const CPlusPlus::Snapshot &snapshot);
    void clear(const QString &comparingFile);
    void insert(const QString &comparingFile);

    static int computeKey(const QString &referenceFile, const QString &comparingFile);

    QMultiMap<int, QString> m_filePriorityCache;
    QSet<QString> m_fileMetaCache;
    QString m_referenceFile;
};

} // namespace CppTools

#endif // SYMBOLFINDER_H

