// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppmodelmanager.h"

#include <utils/futuresynchronizer.h>

#include <QFuture>

namespace Utils { class SearchResultItem; }

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
        Utils::FindFlags flags;
        SymbolTypes types;
        SearchScope scope;
    };

    SymbolSearcher(const SymbolSearcher::Parameters &parameters, const QSet<QString> &fileNames);
    void runSearch(QPromise<Utils::SearchResultItem> &promise);

private:
    const CPlusPlus::Snapshot m_snapshot;
    const Parameters m_parameters;
    const QSet<QString> m_fileNames;
};

class CPPEDITOR_EXPORT CppIndexingSupport
{
public:
    static bool isFindErrorsIndexingActive();

    QFuture<void> refreshSourceFiles(const QSet<QString> &sourceFiles,
                                     CppModelManager::ProgressNotificationMode mode);
private:
    Utils::FutureSynchronizer m_synchronizer;
};

} // namespace CppEditor

Q_DECLARE_METATYPE(CppEditor::SymbolSearcher::SearchScope)
Q_DECLARE_METATYPE(CppEditor::SymbolSearcher::Parameters)
Q_DECLARE_METATYPE(CppEditor::SymbolSearcher::SymbolTypes)
