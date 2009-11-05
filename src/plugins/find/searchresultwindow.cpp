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
#include "searchresulttreeitems.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtGui/QListWidget>
#include <QtGui/QToolButton>
#include <QtGui/QLineEdit>
#include <QtGui/QStackedWidget>
#include <QtGui/QLabel>

using namespace Find;
using namespace Find::Internal;

static const QString SETTINGSKEYSECTIONNAME("SearchResults");
static const QString SETTINGSKEYEXPANDRESULTS("ExpandResults");


SearchResultWindow::SearchResultWindow()
    : m_currentSearch(0),
    m_isShowingReplaceUI(false),
    m_focusReplaceEdit(false)
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
    m_replaceButton->setToolTip(tr("Replace all occurrences"));
    m_replaceButton->setText(tr("Replace"));
    m_replaceButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_replaceButton->setAutoRaise(true);
    m_replaceTextEdit->setTabOrder(m_replaceTextEdit, m_searchResultTreeView);

    connect(m_searchResultTreeView, SIGNAL(jumpToSearchResult(int,bool)),
            this, SLOT(handleJumpToSearchResult(int,bool)));
    connect(m_expandCollapseToolButton, SIGNAL(toggled(bool)), this, SLOT(handleExpandCollapseToolButton(bool)));
    connect(m_replaceTextEdit, SIGNAL(returnPressed()), this, SLOT(handleReplaceButton()));
    connect(m_replaceButton, SIGNAL(clicked()), this, SLOT(handleReplaceButton()));

    readSettings();
    setShowReplaceUI(false);
}

SearchResultWindow::~SearchResultWindow()
{
    writeSettings();
    delete m_currentSearch;
    m_currentSearch = 0;
    delete m_widget;
    m_widget = 0;
    m_items.clear();
}

void SearchResultWindow::setTextToReplace(const QString &textToReplace)
{
    m_replaceTextEdit->setText(textToReplace);
}

QString SearchResultWindow::textToReplace() const
{
    return m_replaceTextEdit->text();
}

void SearchResultWindow::setShowReplaceUI(bool show)
{
    m_searchResultTreeView->model()->setShowReplaceUI(show);
    m_replaceLabel->setVisible(show);
    m_replaceTextEdit->setVisible(show);
    m_replaceButton->setVisible(show);
    m_isShowingReplaceUI = show;
}

void SearchResultWindow::handleReplaceButton()
{
    QTC_ASSERT(m_currentSearch, return);
    // check if button is actually enabled, because this is also triggered
    // by pressing return in replace line edit
    if (m_replaceButton->isEnabled())
        m_currentSearch->replaceButtonClicked(m_replaceTextEdit->text(), checkedItems());
}

QList<SearchResultItem> SearchResultWindow::checkedItems() const
{
    QList<SearchResultItem> result;
    SearchResultTreeModel *model = m_searchResultTreeView->model();
    const int fileCount = model->rowCount(QModelIndex());
    for (int i = 0; i < fileCount; ++i) {
        QModelIndex fileIndex = model->index(i, 0, QModelIndex());
        SearchResultFile *fileItem = static_cast<SearchResultFile *>(fileIndex.internalPointer());
        Q_ASSERT(fileItem != 0);
        for (int rowIndex = 0; rowIndex < fileItem->childrenCount(); ++rowIndex) {
            QModelIndex textIndex = model->index(rowIndex, 0, fileIndex);
            SearchResultTextRow *rowItem = static_cast<SearchResultTextRow *>(textIndex.internalPointer());
            if (rowItem->checkState())
                result << m_items.at(rowItem->index());
        }
    }
    return result;
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

SearchResult *SearchResultWindow::startNewSearch(SearchMode searchOrSearchAndReplace)
{
    clearContents();
    setShowReplaceUI(searchOrSearchAndReplace != SearchOnly);
    delete m_currentSearch;
    m_currentSearch = new SearchResult;
    return m_currentSearch;
}

void SearchResultWindow::finishSearch()
{
    if (m_items.count()) {
        m_replaceButton->setEnabled(true);
    } else {
        showNoMatchesFound();
    }
}

void SearchResultWindow::clearContents()
{
    m_replaceTextEdit->setEnabled(false);
    m_replaceButton->setEnabled(false);
    m_replaceTextEdit->clear();
    m_searchResultTreeView->clear();
    m_items.clear();
    m_widget->setCurrentWidget(m_searchResultTreeView);
    navigateStateChanged();
}

void SearchResultWindow::showNoMatchesFound()
{
    m_replaceTextEdit->setEnabled(false);
    m_replaceButton->setEnabled(false);
    m_widget->setCurrentWidget(m_noMatchesFoundDisplay);
}

bool SearchResultWindow::isEmpty() const
{
    return (m_searchResultTreeView->model()->rowCount() < 1);
}

int SearchResultWindow::numberOfResults() const
{
    return m_items.count();
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
                    || m_widget->focusWidget() == m_replaceTextEdit
                    || m_focusReplaceEdit) {
                m_replaceTextEdit->setFocus();
                m_replaceTextEdit->selectAll();
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

void SearchResultWindow::handleJumpToSearchResult(int index, bool /* checked */)
{
    QTC_ASSERT(m_currentSearch, return);
    m_currentSearch->activated(m_items.at(index));
}

void SearchResultWindow::addResult(const QString &fileName, int lineNumber, const QString &rowText,
    int searchTermStart, int searchTermLength, const QVariant &userData)
{
    //qDebug()<<"###"<<fileName;
    m_widget->setCurrentWidget(m_searchResultTreeView);
    int index = m_items.size();
    SearchResultItem item;
    item.fileName = fileName;
    item.lineNumber = lineNumber;
    item.lineText = rowText;
    item.searchTermStart = searchTermStart;
    item.searchTermLength = searchTermLength;
    item.userData = userData;
    item.index = index;
    m_items.append(item);
    m_searchResultTreeView->appendResultLine(index, fileName, lineNumber, rowText, searchTermStart, searchTermLength);
    if (index == 0) {
        m_replaceTextEdit->setEnabled(true);
        // We didn't have an item before, set the focus to the search widget
        m_focusReplaceEdit = true;
        setFocus();
        m_focusReplaceEdit = false;
        m_searchResultTreeView->selectionModel()->select(m_searchResultTreeView->model()->index(0, 0, QModelIndex()), QItemSelectionModel::Select);
        emit navigateStateChanged();
    }
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
    return m_items.count() > 0;
}

bool SearchResultWindow::canPrevious()
{
    return m_items.count() > 0;
}

void SearchResultWindow::goToNext()
{
    if (m_items.count() == 0)
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
