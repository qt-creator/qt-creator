// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cppeditorconstants.h"

#include <coreplugin/find/searchresultwindow.h>
#include <cplusplus/FindUsages.h>
#include <utils/filepath.h>
#include <utils/link.h>
#include <utils/searchresultitem.h>

#include <QFuture>
#include <QPointer>

#include <functional>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace Core { class SearchResult; }

namespace CppEditor {
class CppModelManager;

Utils::SearchResultColor::Style CPPEDITOR_EXPORT
colorStyleForUsageType(CPlusPlus::Usage::Tags tags);

class CPPEDITOR_EXPORT CppSearchResultFilter : public Core::SearchResultFilter
{
    QWidget *createWidget() override;
    bool matches(const Utils::SearchResultItem &item) const override;

    void setValue(bool &member, bool value);

    bool m_showReads = true;
    bool m_showWrites = true;
    bool m_showDecls = true;
    bool m_showOther = true;
};

namespace Internal {

class CppFindReferencesParameters
{
public:
    QList<QByteArray> symbolId;
    Utils::FilePath symbolFilePath;
    QString prettySymbolName;
    Utils::FilePaths filesToRename;
    bool categorize = false;
    bool preferLowerCaseFileNames = Constants::LOWERCASE_CPPFILES_DEFAULT;
};

class CppFindReferences: public QObject
{
    Q_OBJECT

public:
    explicit CppFindReferences(CppModelManager *modelManager);
    ~CppFindReferences() override;

    QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) const;

public:
    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);
    void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                      const QString &replacement = QString(),
                      const std::function<void()> &callback = {});

    void findMacroUses(const CPlusPlus::Macro &macro);
    void renameMacroUses(const CPlusPlus::Macro &macro, const QString &replacement = QString());

    void checkUnused(Core::SearchResult *search, const Utils::Link &link, CPlusPlus::Symbol *symbol,
                     const CPlusPlus::LookupContext &context, const Utils::LinkHandler &callback);

private:
    void setupSearch(Core::SearchResult *search);
    void onReplaceButtonClicked(Core::SearchResult *search, const QString &text,
                                const Utils::SearchResultItems &items, bool preserveCase);
    void searchAgain(Core::SearchResult *search);

    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                    const QString &replacement, const std::function<void ()> &callback,
                    bool replace);
    void findMacroUses(const CPlusPlus::Macro &macro, const QString &replacement,
                       bool replace);
    void findAll_helper(Core::SearchResult *search, CPlusPlus::Symbol *symbol,
                        const CPlusPlus::LookupContext &context, bool categorize);
    void createWatcher(const QFuture<CPlusPlus::Usage> &future, Core::SearchResult *search);
    CPlusPlus::Symbol *findSymbol(const CppFindReferencesParameters &parameters,
                    const CPlusPlus::Snapshot &snapshot, CPlusPlus::LookupContext *context);

private:
    QPointer<CppModelManager> m_modelManager;
};

} // namespace Internal
} // namespace CppEditor

Q_DECLARE_METATYPE(CppEditor::Internal::CppFindReferencesParameters)
