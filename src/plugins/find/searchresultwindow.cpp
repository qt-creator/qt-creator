/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "searchresultwindow.h"
#include "searchresulttreemodel.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtGui/QListWidget>
#include <QtGui/QToolButton>

using namespace Find;
using namespace Find::Internal;

static const QString SETTINGSKEYSECTIONNAME("SearchResults");
static const QString SETTINGSKEYEXPANDRESULTS("ExpandResults");

SearchResultWindow::SearchResultWindow(Core::ICore *core):
    m_core(core),
    m_widget(new QStackedWidget())
{
    m_widget->setWindowTitle(name());

    m_searchResultTreeView = new SearchResultTreeView(m_widget);
    m_searchResultTreeView->setUniformRowHeights(true);
    m_searchResultTreeView->setFrameStyle(QFrame::NoFrame);
    m_widget->addWidget(m_searchResultTreeView);

    m_noMatchesFoundDisplay = new QListWidget(m_widget);
    m_noMatchesFoundDisplay->addItem(tr("No matches found!"));
    m_noMatchesFoundDisplay->setFrameStyle(QFrame::NoFrame);
    m_widget->addWidget(m_noMatchesFoundDisplay);

    m_expandCollapseToolButton = new QToolButton(m_widget);
    m_expandCollapseToolButton->setAutoRaise(true);
    m_expandCollapseToolButton->setCheckable(true);
    m_expandCollapseToolButton->setIcon(QIcon(":/find/images/expand.png"));
    m_expandCollapseToolButton->setToolTip(tr("Expand All"));

    connect(m_searchResultTreeView, SIGNAL(jumpToSearchResult(int,const QString&,int,int,int)),
            this, SLOT(handleJumpToSearchResult(int,const QString&,int,int,int)));
    connect(m_expandCollapseToolButton, SIGNAL(toggled(bool)), this, SLOT(handleExpandCollapseToolButton(bool)));

    readSettings();
}

SearchResultWindow::~SearchResultWindow()
{
    writeSettings();
    delete m_widget;
    m_widget = 0;
    qDeleteAll(m_items);
    m_items.clear();
}

bool SearchResultWindow::hasFocus()
{
    return m_searchResultTreeView->hasFocus();
}

bool SearchResultWindow::canFocus()
{
    return !m_items.isEmpty();
}

void SearchResultWindow::setFocus()
{
    if (!m_items.isEmpty())
        m_searchResultTreeView->setFocus();
}

void SearchResultWindow::visibilityChanged(bool /*visible*/)
{
}

QWidget *SearchResultWindow::outputWidget(QWidget *)
{
    return m_widget;
}

QList<QWidget*> SearchResultWindow::toolBarWidgets(void) const
{
    return QList<QWidget*>() << m_expandCollapseToolButton;
}

void SearchResultWindow::clearContents()
{
    m_widget->setCurrentWidget(m_searchResultTreeView);
    m_searchResultTreeView->clear();
    qDeleteAll(m_items);
    m_items.clear();
}

void SearchResultWindow::showNoMatchesFound(void)
{
    m_widget->setCurrentWidget(m_noMatchesFoundDisplay);
}

bool SearchResultWindow::isEmpty() const
{
    return (m_searchResultTreeView->model()->rowCount() < 1);
}

int SearchResultWindow::numberOfResults() const
{
    return m_searchResultTreeView->model()->rowCount();
}

void SearchResultWindow::handleJumpToSearchResult(int index, const QString &fileName, int lineNumber,
    int searchTermStart, int searchTermLength)
{
    Q_UNUSED(searchTermLength);
    ResultWindowItem *item = m_items.at(index);
    emit item->activated(fileName, lineNumber, searchTermStart);
}

ResultWindowItem *SearchResultWindow::addResult(const QString &fileName, int lineNumber, const QString &rowText,
    int searchTermStart, int searchTermLength)
{
    m_widget->setCurrentWidget(m_searchResultTreeView);
    int index = m_items.size();
    ResultWindowItem *item = new ResultWindowItem;
    m_items.append(item);
    m_searchResultTreeView->appendResultLine(index, fileName, lineNumber, rowText, searchTermStart, searchTermLength);
    if (index == 0) {
        // We didn't have an item before, set the focus to the m_searchResultTreeView
        m_searchResultTreeView->setFocus();
        m_searchResultTreeView->selectionModel()->select(m_searchResultTreeView->model()->index(0, 0, QModelIndex()), QItemSelectionModel::Select);
    }

    return item;
}

void SearchResultWindow::handleExpandCollapseToolButton(bool checked)
{
    m_searchResultTreeView->setAutoExpandResults(checked);
    if (checked)
        m_searchResultTreeView->expandAll();
    else
        m_searchResultTreeView->collapseAll();
}

void SearchResultWindow::readSettings(void)
{
    if (m_core && m_core->settings()) {
        QSettings *s = m_core->settings();
        s->beginGroup(SETTINGSKEYSECTIONNAME);
        m_expandCollapseToolButton->setChecked(s->value(SETTINGSKEYEXPANDRESULTS, m_initiallyExpand).toBool());
        s->endGroup();
    }
}

void SearchResultWindow::writeSettings(void)
{
    if (m_core && m_core->settings()) {
        QSettings *s = m_core->settings();
        s->beginGroup(SETTINGSKEYSECTIONNAME);
        s->setValue(SETTINGSKEYEXPANDRESULTS, m_expandCollapseToolButton->isChecked());
        s->endGroup();
    }
}

int SearchResultWindow::priorityInStatusBar() const
{
    return 80;
}
