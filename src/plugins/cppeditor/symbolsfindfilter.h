// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "searchsymbols.h"

#include <coreplugin/find/ifindfilter.h>
#include <coreplugin/find/searchresultwindow.h>

#include <solutions/tasking/tasktreerunner.h>

#include <QCheckBox>
#include <QPointer>
#include <QRadioButton>
#include <QWidget>

namespace Core { class SearchResult; }
namespace Utils { class SearchResultItem; }

namespace CppEditor {
namespace Internal {

class SymbolsFindFilter : public Core::IFindFilter
{
    Q_OBJECT

public:
    using SearchScope = SymbolSearcher::SearchScope;

public:
    SymbolsFindFilter();

    QString id() const override;
    QString displayName() const override;
    bool isEnabled() const override;

    void findAll(const QString &txt, Utils::FindFlags findFlags) override;

    QWidget *createConfigWidget() override;
    Utils::Store save() const override;
    void restore(const Utils::Store &s) override;

    void setSymbolsToSearch(const SearchSymbols::SymbolTypes &types) { m_symbolsToSearch = types; }
    SearchSymbols::SymbolTypes symbolsToSearch() const { return m_symbolsToSearch; }

    void setSearchScope(SearchScope scope) { m_scope = scope; }
    SearchScope searchScope() const { return m_scope; }

    // deprecated
    QByteArray settingsKey() const override;

signals:
    void symbolsToSearchChanged();

private:
    void openEditor(const Utils::SearchResultItem &item);

    void onTaskStarted(Utils::Id type);
    void onAllTasksFinished(Utils::Id type);

    QString label() const;
    QString toolTip(Utils::FindFlags findFlags) const;
    void startSearch(Core::SearchResult *search);

    bool m_enabled;
    QPointer<Core::SearchResult> m_currentSearch;
    SearchSymbols::SymbolTypes m_symbolsToSearch;
    SearchScope m_scope;
    Tasking::ParallelTaskTreeRunner m_taskTreeRunner;
};

class SymbolsFindFilterConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SymbolsFindFilterConfigWidget(SymbolsFindFilter *filter);

private:
    void setState() const;
    void getState();

    SymbolsFindFilter *m_filter;

    QCheckBox *m_typeClasses;
    QCheckBox *m_typeMethods;
    QCheckBox *m_typeEnums;
    QCheckBox *m_typeDeclarations;

    QRadioButton *m_searchGlobal;
    QRadioButton *m_searchProjectsOnly;
    QButtonGroup *m_searchGroup;
};

} // Internal
} // CppEditor
