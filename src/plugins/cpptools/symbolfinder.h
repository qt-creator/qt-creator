/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cpptools_global.h"

#include "cppfileiterationorder.h"

#include <QHash>
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

namespace CppTools {

class CPPTOOLS_EXPORT SymbolFinder
{
public:
    SymbolFinder();

    CPlusPlus::Function *findMatchingDefinition(CPlusPlus::Symbol *symbol,
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

} // namespace CppTools
