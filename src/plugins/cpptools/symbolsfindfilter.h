/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SYMBOLSFINDFILTER_H
#define SYMBOLSFINDFILTER_H

#include "searchsymbols.h"

#include <find/ifindfilter.h>

#include <QFutureWatcher>
#include <QPointer>
#include <QWidget>
#include <QCheckBox>
#include <QRadioButton>

namespace CppTools {
namespace Internal {

class CppModelManager;

class SymbolsFindFilter : public Find::IFindFilter
{
    Q_OBJECT

public:
    typedef SymbolSearcher::SearchScope SearchScope;

public:
    explicit SymbolsFindFilter(CppModelManager *manager);

    QString id() const;
    QString displayName() const;
    bool isEnabled() const;
    Find::FindFlags supportedFindFlags() const;

    void findAll(const QString &txt, Find::FindFlags findFlags);

    QWidget *createConfigWidget();
    void writeSettings(QSettings *settings);
    void readSettings(QSettings *settings);

    void setSymbolsToSearch(SearchSymbols::SymbolTypes types) { m_symbolsToSearch = types; }
    SearchSymbols::SymbolTypes symbolsToSearch() const { return m_symbolsToSearch; }

    void setSearchScope(SearchScope scope) { m_scope = scope; }
    SearchScope searchScope() const { return m_scope; }

signals:
    void symbolsToSearchChanged();

private slots:
    void openEditor(const Find::SearchResultItem &item);

    void addResults(int begin, int end);
    void finish();
    void cancel();
    void setPaused(bool paused);
    void onTaskStarted(const QString &type);
    void onAllTasksFinished(const QString &type);
    void searchAgain();

private:
    QString label() const;
    QString toolTip(Find::FindFlags findFlags) const;
    void startSearch(Find::SearchResult *search);

    CppModelManager *m_manager;
    bool m_enabled;
    QMap<QFutureWatcher<Find::SearchResultItem> *, QPointer<Find::SearchResult> > m_watchers;
    QPointer<Find::SearchResult> m_currentSearch;
    SearchSymbols::SymbolTypes m_symbolsToSearch;
    SearchScope m_scope;
};

class SymbolsFindFilterConfigWidget : public QWidget
{
    Q_OBJECT
public:
    SymbolsFindFilterConfigWidget(SymbolsFindFilter *filter);

private slots:
    void setState() const;
    void getState();

private:
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

#endif // SYMBOLSFINDFILTER_H
