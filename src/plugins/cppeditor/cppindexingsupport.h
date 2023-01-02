// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppmodelmanager.h"

#include <coreplugin/find/textfindconstants.h>
#include <utils/futuresynchronizer.h>

#include <QFuture>
#include <QStringList>

namespace Core { class SearchResultItem; }

namespace CppEditor {

class CPPEDITOR_EXPORT SymbolSearcher: public QObject
{
    Q_OBJECT

public:
    enum SymbolType {
        Classes      = 0x1,
        Functions    = 0x2,
        Enums        = 0x4,
        Declarations = 0x8,
        TypeAliases  = 0x16,
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
    SymbolSearcher(QObject *parent = nullptr);
    ~SymbolSearcher() override = 0;
    virtual void runSearch(QFutureInterface<Core::SearchResultItem> &future) = 0;
};

class CPPEDITOR_EXPORT CppIndexingSupport
{
public:
    CppIndexingSupport();
    ~CppIndexingSupport();

    static bool isFindErrorsIndexingActive();

    QFuture<void> refreshSourceFiles(const QSet<QString> &sourceFiles,
                                     CppModelManager::ProgressNotificationMode mode);
    SymbolSearcher *createSymbolSearcher(const SymbolSearcher::Parameters &parameters,
                                         const QSet<QString> &fileNames);
private:
    Utils::FutureSynchronizer m_synchronizer;
};

} // namespace CppEditor

Q_DECLARE_METATYPE(CppEditor::SymbolSearcher::SearchScope)
Q_DECLARE_METATYPE(CppEditor::SymbolSearcher::Parameters)
Q_DECLARE_METATYPE(CppEditor::SymbolSearcher::SymbolTypes)
