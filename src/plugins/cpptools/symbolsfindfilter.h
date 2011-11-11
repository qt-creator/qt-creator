/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SYMBOLSFINDFILTER_H
#define SYMBOLSFINDFILTER_H

#include "searchsymbols.h"

#include <find/ifindfilter.h>
#include <find/searchresultwindow.h>

#include <QtCore/QFutureInterface>
#include <QtCore/QFutureWatcher>
#include <QtCore/QPointer>
#include <QtGui/QWidget>
#include <QtGui/QCheckBox>
#include <QtGui/QRadioButton>

namespace CppTools {
namespace Internal {

class CppModelManager;

class SymbolsFindFilter : public Find::IFindFilter
{
    Q_OBJECT
public:
    enum SearchScope {
        SearchProjectsOnly,
        SearchGlobal
    };

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
    void onTaskStarted(const QString &type);
    void onAllTasksFinished(const QString &type);

private:
    QString label() const;
    QString toolTip(Find::FindFlags findFlags) const;

    CppModelManager *m_manager;
    bool m_isRunning;
    bool m_enabled;
    QFutureWatcher<Find::SearchResultItem> m_watcher;
    QPointer<Find::SearchResult> m_currentSearch;
    SearchSymbols::SymbolTypes m_symbolsToSearch;
    SearchSymbols m_search;
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
