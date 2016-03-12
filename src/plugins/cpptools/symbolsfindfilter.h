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

#include "searchsymbols.h"

#include <coreplugin/find/ifindfilter.h>

#include <QFutureWatcher>
#include <QPointer>
#include <QWidget>
#include <QCheckBox>
#include <QRadioButton>

namespace Core { class SearchResult; }

namespace CppTools {

class CppModelManager;

namespace Internal {

class SymbolsFindFilter : public Core::IFindFilter
{
    Q_OBJECT

public:
    typedef SymbolSearcher::SearchScope SearchScope;

public:
    explicit SymbolsFindFilter(CppModelManager *manager);

    QString id() const;
    QString displayName() const;
    bool isEnabled() const;
    Core::FindFlags supportedFindFlags() const;

    void findAll(const QString &txt, Core::FindFlags findFlags);

    QWidget *createConfigWidget();
    void writeSettings(QSettings *settings);
    void readSettings(QSettings *settings);

    void setSymbolsToSearch(const SearchSymbols::SymbolTypes &types) { m_symbolsToSearch = types; }
    SearchSymbols::SymbolTypes symbolsToSearch() const { return m_symbolsToSearch; }

    void setSearchScope(SearchScope scope) { m_scope = scope; }
    SearchScope searchScope() const { return m_scope; }

signals:
    void symbolsToSearchChanged();

private:
    void openEditor(const Core::SearchResultItem &item);

    void addResults(int begin, int end);
    void finish();
    void cancel();
    void setPaused(bool paused);
    void onTaskStarted(Core::Id type);
    void onAllTasksFinished(Core::Id type);
    void searchAgain();

    QString label() const;
    QString toolTip(Core::FindFlags findFlags) const;
    void startSearch(Core::SearchResult *search);

    CppModelManager *m_manager;
    bool m_enabled;
    QMap<QFutureWatcher<Core::SearchResultItem> *, QPointer<Core::SearchResult> > m_watchers;
    QPointer<Core::SearchResult> m_currentSearch;
    SearchSymbols::SymbolTypes m_symbolsToSearch;
    SearchScope m_scope;
};

class SymbolsFindFilterConfigWidget : public QWidget
{
    Q_OBJECT
public:
    SymbolsFindFilterConfigWidget(SymbolsFindFilter *filter);

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
} // CppTools
