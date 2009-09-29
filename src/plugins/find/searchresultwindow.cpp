/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "searchresultwindow.h"
#include "searchresulttreemodel.h"

#include <coreplugin/icore.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtGui/QListWidget>
#include <QtGui/QToolButton>
#include <QtGui/QLineEdit>

using namespace Find;
using namespace Find::Internal;

static const QString SETTINGSKEYSECTIONNAME("SearchResults");
static const QString SETTINGSKEYEXPANDRESULTS("ExpandResults");


void ResultWindowItem::setData(const QVariant &data)
{
    m_data = data;
}

QVariant ResultWindowItem::data() const
{
    return m_data;
}

SearchResultWindow::SearchResultWindow()
    : m_isShowingReplaceUI(false)
{
    m_widget = new QStackedWidget;
    m_widget->setWindowTitle(name());

    m_searchResultTreeView = new SearchResultTreeView(m_widget);
    m_searchResultTreeView->setFrameStyle(QFrame::NoFrame);
    m_searchResultTreeView->setAttribute(Qt::WA_MacShowFocusRect, false);
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

    m_replaceLabel = new QLabel(tr("Replace with:"), m_widget);
    m_replaceLabel->setContentsMargins(12, 0, 5, 0);
    m_replaceTextEdit = new QLineEdit(m_widget);
    m_replaceButton = new QToolButton(m_widget);
    m_replaceButton->setToolTip(tr("Replace all occurances"));
    m_replaceButton->setText(tr("Replace"));
    m_replaceButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_replaceButton->setAutoRaise(true);
    m_replaceTextEdit->setTabOrder(m_replaceTextEdit, m_searchResultTreeView);

    connect(m_searchResultTreeView, SIGNAL(jumpToSearchResult(int,const QString&,int,int,int)),
            this, SLOT(handleJumpToSearchResult(int,const QString&,int,int,int)));
    connect(m_expandCollapseToolButton, SIGNAL(toggled(bool)), this, SLOT(handleExpandCollapseToolButton(bool)));
    connect(m_replaceButton, SIGNAL(clicked()), this, SLOT(handleReplaceButton()));

    readSettings();
    setShowReplaceUI(false);
}

SearchResultWindow::~SearchResultWindow()
{
    writeSettings();
    delete m_widget;
    m_widget = 0;
    qDeleteAll(m_items);
    m_items.clear();
}

void SearchResultWindow::setShowReplaceUI(bool show)
{
    m_searchResultTreeView->model()->setShowReplaceUI(show);
    m_replaceLabel->setVisible(show);
    m_replaceTextEdit->setVisible(show);
    m_replaceButton->setVisible(show);
//  TODO  m_searchResultTreeView->setShowCheckboxes(show);
    m_isShowingReplaceUI = show;
}

bool SearchResultWindow::isShowingReplaceUI() const
{
    return m_isShowingReplaceUI;
}

void SearchResultWindow::handleReplaceButton()
{
    emit replaceButtonClicked(m_replaceTextEdit->text());
}

QList<ResultWindowItem *> SearchResultWindow::selectedItems() const
{
    QList<ResultWindowItem *> items;
    // TODO
//    foreach (ResultWindowItem *item, m_items) {
//        if (item->isSelected)
//            items << item;
//    }
    return items;
}

void SearchResultWindow::visibilityChanged(bool /*visible*/)
{
}

QWidget *SearchResultWindow::outputWidget(QWidget *)
{
    return m_widget;
}

QList<QWidget*> SearchResultWindow::toolBarWidgets() const
{
    return QList<QWidget*>() << m_expandCollapseToolButton << m_replaceLabel << m_replaceTextEdit << m_replaceButton;
}

void SearchResultWindow::clearContents()
{
    setShowReplaceUI(false);
    m_widget->setCurrentWidget(m_searchResultTreeView);
    m_searchResultTreeView->clear();
    qDeleteAll(m_items);
    m_items.clear();
    navigateStateChanged();
}

void SearchResultWindow::showNoMatchesFound()
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

bool SearchResultWindow::hasFocus()
{
    return m_searchResultTreeView->hasFocus() || (m_isShowingReplaceUI && m_replaceTextEdit->hasFocus());
}

bool SearchResultWindow::canFocus()
{
    return !m_items.isEmpty();
}

void SearchResultWindow::setFocus()
{
    if (!m_items.isEmpty()) {
        if (!m_isShowingReplaceUI) {
            m_searchResultTreeView->setFocus();
        } else {
            if (!m_widget->focusWidget()
                    || m_widget->focusWidget() == m_replaceTextEdit) {
                m_replaceTextEdit->setFocus();
            } else {
                m_searchResultTreeView->setFocus();
            }
        }
    }
}

void SearchResultWindow::setTextEditorFont(const QFont &font)
{
    m_searchResultTreeView->setTextEditorFont(font);
}

void SearchResultWindow::handleJumpToSearchResult(int index, const QString &fileName, int lineNumber,
    int searchTermStart, int searchTermLength)
{
    Q_UNUSED(searchTermLength)
    ResultWindowItem *item = m_items.at(index);
    emit item->activated(fileName, lineNumber, searchTermStart);
}

ResultWindowItem *SearchResultWindow::addResult(const QString &fileName, int lineNumber, const QString &rowText,
    int searchTermStart, int searchTermLength)
{
    //qDebug()<<"###"<<fileName;
    m_widget->setCurrentWidget(m_searchResultTreeView);
    int index = m_items.size();
    ResultWindowItem *item = new ResultWindowItem;
    m_items.append(item);
    m_searchResultTreeView->appendResultLine(index, fileName, lineNumber, rowText, searchTermStart, searchTermLength);
    if (index == 0) {
        // We didn't have an item before, set the focus to the m_searchResultTreeView
        setFocus();
        m_searchResultTreeView->selectionModel()->select(m_searchResultTreeView->model()->index(0, 0, QModelIndex()), QItemSelectionModel::Select);
        emit navigateStateChanged();
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

void SearchResultWindow::readSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    if (s) {
        s->beginGroup(SETTINGSKEYSECTIONNAME);
        m_expandCollapseToolButton->setChecked(s->value(SETTINGSKEYEXPANDRESULTS, m_initiallyExpand).toBool());
        s->endGroup();
    }
}

void SearchResultWindow::writeSettings()
{
    QSettings *s = Core::ICore::instance()->settings();
    if (s) {
        s->beginGroup(SETTINGSKEYSECTIONNAME);
        s->setValue(SETTINGSKEYEXPANDRESULTS, m_expandCollapseToolButton->isChecked());
        s->endGroup();
    }
}

int SearchResultWindow::priorityInStatusBar() const
{
    return 80;
}

bool SearchResultWindow::canNext()
{
    return m_searchResultTreeView->model()->rowCount();
}

bool SearchResultWindow::canPrevious()
{
    return m_searchResultTreeView->model()->rowCount();
}

void SearchResultWindow::goToNext()
{
    if (!m_searchResultTreeView->model()->rowCount())
        return;
    QModelIndex idx = m_searchResultTreeView->model()->next(m_searchResultTreeView->currentIndex());
    if (idx.isValid()) {
        m_searchResultTreeView->setCurrentIndex(idx);
        m_searchResultTreeView->emitJumpToSearchResult(idx);
    }
}
void SearchResultWindow::goToPrev()
{
    if (!m_searchResultTreeView->model()->rowCount())
        return;
    QModelIndex idx = m_searchResultTreeView->model()->prev(m_searchResultTreeView->currentIndex());
    if (idx.isValid()) {
        m_searchResultTreeView->setCurrentIndex(idx);
        m_searchResultTreeView->emitJumpToSearchResult(idx);
    }
}

bool SearchResultWindow::canNavigate()
{
    return true;
}
