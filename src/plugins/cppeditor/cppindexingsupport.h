// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppmodelmanager.h"

#include <utils/futuresynchronizer.h>

#include <QFuture>

#include <functional>

namespace Utils { class SearchResultItem; }

namespace CppEditor::Internal {

enum class SymbolType {
    Classes      = 0x1,
    Functions    = 0x2,
    Enums        = 0x4,
    Declarations = 0x8,
    TypeAliases  = 0x16,
    AllTypes     = Classes | Functions | Enums | Declarations
};
Q_DECLARE_FLAGS(SymbolTypes, SymbolType)
Q_DECLARE_OPERATORS_FOR_FLAGS(SymbolTypes)

enum SearchScope {
    SearchProjectsOnly,
    SearchGlobal
};

struct SearchParameters
{
    QString text;
    Utils::FindFlags flags;
    SymbolTypes types;
    SearchScope scope;
};

CPPEDITOR_EXPORT void searchForSymbols(QPromise<Utils::SearchResultItem> &promise,
                                       const CPlusPlus::Snapshot &snapshot,
                                       const SearchParameters &parameters,
                                       const QSet<Utils::FilePath> &filePaths);

CPPEDITOR_EXPORT bool isFindErrorsIndexingActive();

CPPEDITOR_EXPORT QFuture<void> refreshSourceFiles(
    const std::function<QSet<Utils::FilePath>()> &sourceFiles,
    CppModelManager::ProgressNotificationMode mode);

} // namespace CppEditor::Internal

Q_DECLARE_METATYPE(CppEditor::Internal::SearchScope)
Q_DECLARE_METATYPE(CppEditor::Internal::SearchParameters)
Q_DECLARE_METATYPE(CppEditor::Internal::SymbolTypes)
