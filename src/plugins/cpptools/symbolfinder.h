#ifndef SYMBOLFINDER_H
#define SYMBOLFINDER_H

#include "cpptools_global.h"

#include <CppDocument.h>
#include <CPlusPlusForwardDeclarations.h>

#include <QHash>
#include <QStringList>
#include <QMultiMap>
#include <QSet>

namespace CppTools {

class CPPTOOLS_EXPORT SymbolFinder
{
public:
    SymbolFinder();

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
    QStringList fileIterationOrder(const QString &referenceFile,
                                   const CPlusPlus::Snapshot &snapshot);

    void checkCacheConsistency(const QString &referenceFile, const CPlusPlus::Snapshot &snapshot);
    void clearCache(const QString &referenceFile, const QString &comparingFile);
    void insertCache(const QString &referenceFile, const QString &comparingFile);

    void trackCacheUse(const QString &referenceFile);

    static int computeKey(const QString &referenceFile, const QString &comparingFile);

    QHash<QString, QMultiMap<int, QString> > m_filePriorityCache;
    QHash<QString, QSet<QString> > m_fileMetaCache;
    QStringList m_recent;
};

} // namespace CppTools

#endif // SYMBOLFINDER_H

