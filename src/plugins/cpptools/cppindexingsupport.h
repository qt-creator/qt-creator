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

#include "cppmodelmanager.h"

#include <coreplugin/find/textfindconstants.h>

#include <QFuture>
#include <QStringList>

namespace Core { class SearchResultItem; }

namespace CppTools {

class CPPTOOLS_EXPORT SymbolSearcher: public QObject
{
    Q_OBJECT

public:
    enum SymbolType {
        Classes      = 0x1,
        Functions    = 0x2,
        Enums        = 0x4,
        Declarations = 0x8
    };

    Q_DECLARE_FLAGS(SymbolTypes, SymbolType)

    enum SearchScope {
        SearchProjectsOnly,
        SearchGlobal
    };

    struct Parameters
    {
        QString text;
        Core::FindFlags flags;
        SymbolTypes types;
        SearchScope scope;
    };


public:
    SymbolSearcher(QObject *parent = 0);
    virtual ~SymbolSearcher() = 0;
    virtual void runSearch(QFutureInterface<Core::SearchResultItem> &future) = 0;
};


class CPPTOOLS_EXPORT CppIndexingSupport
{
public:
    virtual ~CppIndexingSupport() = 0;

    virtual QFuture<void> refreshSourceFiles(const QFutureInterface<void> &superFuture,
                                             const QSet<QString> &sourceFiles,
                                             CppModelManager::ProgressNotificationMode mode) = 0;
    virtual SymbolSearcher *createSymbolSearcher(SymbolSearcher::Parameters parameters,
                                                 QSet<QString> fileNames) = 0;
};

} // namespace CppTools

Q_DECLARE_METATYPE(CppTools::SymbolSearcher::SearchScope)
Q_DECLARE_METATYPE(CppTools::SymbolSearcher::Parameters)
Q_DECLARE_METATYPE(CppTools::SymbolSearcher::SymbolTypes)
