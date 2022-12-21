// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppfileiterationorder.h"

#include <utils/filepath.h>

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
    Utils::FilePaths fileIterationOrder(const Utils::FilePath &referenceFile,
                                        const CPlusPlus::Snapshot &snapshot);
    void checkCacheConsistency(const Utils::FilePath &referenceFile, const CPlusPlus::Snapshot &snapshot);
    void clearCache(const Utils::FilePath &referenceFile, const Utils::FilePath &comparingFile);
    void insertCache(const Utils::FilePath &referenceFile, const Utils::FilePath &comparingFile);

    void trackCacheUse(const Utils::FilePath &referenceFile);

    QHash<Utils::FilePath, FileIterationOrder> m_filePriorityCache;
    QHash<Utils::FilePath, QSet<Utils::FilePath> > m_fileMetaCache;
    Utils::FilePaths m_recent;
};

} // namespace CppEditor
