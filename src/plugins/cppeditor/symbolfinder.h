// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppfileiterationorder.h"

#include <QHash>
#include <QSet>
#include <QStringList>

#include <set>

namespace CPlusPlus {
class Class;
class Declaration;
class Function;
class LookupContext;
class Snapshot;
class Symbol;
}

namespace CppEditor {

class CPPEDITOR_EXPORT SymbolFinder
{
public:
    SymbolFinder();

    CPlusPlus::Function *findMatchingDefinition(CPlusPlus::Symbol *symbol,
                                                const CPlusPlus::Snapshot &snapshot,
                                                bool strict = false);

    CPlusPlus::Symbol *findMatchingVarDefinition(CPlusPlus::Symbol *declaration,
                                                 const CPlusPlus::Snapshot &snapshot);

    CPlusPlus::Class *findMatchingClassDeclaration(CPlusPlus::Symbol *declaration,
                                                   const CPlusPlus::Snapshot &snapshot);

    void findMatchingDeclaration(const CPlusPlus::LookupContext &context,
                                 CPlusPlus::Function *functionType,
                                 QList<CPlusPlus::Declaration *> *typeMatch,
                                 QList<CPlusPlus::Declaration *> *argumentCountMatch,
                                 QList<CPlusPlus::Declaration *> *nameMatch);

    QList<CPlusPlus::Declaration *> findMatchingDeclaration(const CPlusPlus::LookupContext &context,
                                                            CPlusPlus::Function *functionType);

    void clearCache();

private:
    QStringList fileIterationOrder(const QString &referenceFile,
                                   const CPlusPlus::Snapshot &snapshot);
    void checkCacheConsistency(const QString &referenceFile, const CPlusPlus::Snapshot &snapshot);
    void clearCache(const QString &referenceFile, const QString &comparingFile);
    void insertCache(const QString &referenceFile, const QString &comparingFile);

    void trackCacheUse(const QString &referenceFile);

    QHash<QString, FileIterationOrder> m_filePriorityCache;
    QHash<QString, QSet<QString> > m_fileMetaCache;
    QStringList m_recent;
};

} // namespace CppEditor
