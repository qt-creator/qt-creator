/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SYMBOLFINDER_H
#define SYMBOLFINDER_H

#include "cpptools_global.h"

#include <cplusplus/CppDocument.h>

#include <QHash>
#include <QStringList>
#include <QMultiMap>
#include <QSet>

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

