/**************************************************************************
**
** Copyright (c) 2013 Dmitry Savchenko
** Copyright (c) 2013 Vasiliy Sorokin
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

#include "todooutputpane.h"
#include "constants.h"
#include "todoitemsmodel.h"

#include <QIcon>
#include <QHeaderView>
#include <QTreeView>
#include <QToolButton>
#include <QButtonGroup>

namespace Todo {
namespace Internal {

TodoOutputPane::TodoOutputPane(TodoItemsModel *todoItemsModel, QObject *parent) :
    IOutputPane(parent),
    m_todoItemsModel(todoItemsModel)
{
    createTreeView();
    createScopeButtons();
    setScanningScope(ScanningScopeCurrentFile); // default
    connect(m_todoItemsModel, SIGNAL(layoutChanged()), SIGNAL(navigateStateUpdate()));
    connect(m_todoItemsModel, SIGNAL(layoutChanged()), SLOT(updateTodoCount()));
}

TodoOutputPane::~TodoOutputPane()
{
    freeTreeView();
    freeScopeButtons();
}

QWidget *TodoOutputPane::outputWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    return m_todoTreeView;
}

QList<QWidget*> TodoOutputPane::toolBarWidgets() const
{
    return QList<QWidget*>()
        << m_spacer
        << m_currentFileButton
        << m_wholeProjectButton;
}

QString TodoOutputPane::displayName() const
{
    return tr(Constants::OUTPUT_PANE_TITLE);
}

int TodoOutputPane::priorityInStatusBar() const
{
    return 1;
}

void TodoOutputPane::clearContents()
{
}

void TodoOutputPane::visibilityChanged(bool visible)
{
    Q_UNUSED(visible)
}

void TodoOutputPane::setFocus()
{
    m_todoTreeView->setFocus();
}

bool TodoOutputPane::hasFocus() const
{
    return m_todoTreeView->hasFocus();
}

bool TodoOutputPane::canFocus() const
{
    return true;
}

bool TodoOutputPane::canNavigate() const
{
    return true;
}

bool TodoOutputPane::canNext() const
{
    return m_todoTreeView->model()->rowCount() > 1;
}

bool TodoOutputPane::canPrevious() const
{
    return m_todoTreeView->model()->rowCount() > 1;
}

void TodoOutputPane::goToNext()
{
    m_todoTreeView->selectionModel()->select(nextModelIndex(), QItemSelectionModel::SelectCurrent);
}

void TodoOutputPane::goToPrev()
{
    m_todoTreeView->selectionModel()->select(previousModelIndex(), QItemSelectionModel::SelectCurrent);
}

void TodoOutputPane::setScanningScope(ScanningScope scanningScope)
{
    if (scanningScope == ScanningScopeCurrentFile)
        m_currentFileButton->setChecked(true);
    else if (scanningScope == ScanningScopeProject)
        m_wholeProjectButton->setChecked(true);
    else
        Q_ASSERT_X(false, "Updating scanning scope buttons", "Unknown scanning scope enum value");
}

void TodoOutputPane::scopeButtonClicked(QAbstractButton* button)
{
    if (button == m_currentFileButton)
        emit scanningScopeChanged(ScanningScopeCurrentFile);
    else if (button == m_wholeProjectButton)
        emit scanningScopeChanged(ScanningScopeProject);
    setBadgeNumber(m_todoItemsModel->rowCount());
}

void TodoOutputPane::todoTreeViewClicked(const QModelIndex &index)
{
    // Create a to-do item and notify that it was clicked on

    int row = index.row();

    TodoItem item;
    item.text = index.sibling(row, Constants::OUTPUT_COLUMN_TEXT).data().toString();
    item.file = index.sibling(row, Constants::OUTPUT_COLUMN_FILE).data().toString();
    item.line = index.sibling(row, Constants::OUTPUT_COLUMN_LINE).data().toInt();
    item.color = index.data(Qt::BackgroundColorRole).value<QColor>();
    item.iconResource = index.sibling(row, Constants::OUTPUT_COLUMN_TEXT).data(Qt::DecorationRole).toString();

    emit todoItemClicked(item);
}

void TodoOutputPane::updateTodoCount()
{
    setBadgeNumber(m_todoItemsModel->rowCount());
}

void TodoOutputPane::createTreeView()
{
    m_todoTreeView = new QTreeView();

    m_todoTreeView->setRootIsDecorated(false);
    m_todoTreeView->setFrameStyle(QFrame::NoFrame);
    m_todoTreeView->setSortingEnabled(true);
    m_todoTreeView->setModel(m_todoItemsModel);
    m_todoTreeView->setAttribute(Qt::WA_MacShowFocusRect, false);

    QHeaderView *header = m_todoTreeView->header();
    header->setResizeMode(Constants::OUTPUT_COLUMN_TEXT, QHeaderView::Stretch);
    header->setResizeMode(Constants::OUTPUT_COLUMN_LINE, QHeaderView::ResizeToContents);
    header->setResizeMode(Constants::OUTPUT_COLUMN_FILE, QHeaderView::ResizeToContents);
    header->setStretchLastSection(false);
    header->setMovable(false);

    connect(m_todoTreeView, SIGNAL(clicked(QModelIndex)), SLOT(todoTreeViewClicked(QModelIndex)));
}

void TodoOutputPane::freeTreeView()
{
    delete m_todoTreeView;
}

void TodoOutputPane::createScopeButtons()
{
    m_currentFileButton = new QToolButton();
    m_currentFileButton->setIcon(QIcon(QLatin1String(Constants::ICON_CURRENT_FILE)));
    m_currentFileButton->setCheckable(true);
    m_currentFileButton->setToolTip(tr("Scan in the current opened file"));

    m_wholeProjectButton = new QToolButton();
    m_wholeProjectButton->setIcon(QIcon(QLatin1String(Constants::ICON_WHOLE_PROJECT)));
    m_wholeProjectButton->setCheckable(true);
    m_wholeProjectButton->setToolTip(tr("Scan in the whole project"));

    m_scopeButtons = new QButtonGroup();
    m_scopeButtons->addButton(m_wholeProjectButton);
    m_scopeButtons->addButton(m_currentFileButton);
    connect(m_scopeButtons, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(scopeButtonClicked(QAbstractButton*)));

    m_spacer = new QWidget;
    m_spacer->setMinimumWidth(Constants::OUTPUT_TOOLBAR_SPACER_WIDTH);
}

void TodoOutputPane::freeScopeButtons()
{
    delete m_currentFileButton;
    delete m_wholeProjectButton;
    delete m_scopeButtons;
    delete m_spacer;
}

QModelIndex TodoOutputPane::selectedModelIndex()
{
    QModelIndexList selectedIndexes = m_todoTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty())
        return QModelIndex();
    else
        // There is only one item selected
        return selectedIndexes.first();
}

QModelIndex TodoOutputPane::nextModelIndex()
{
    QModelIndex indexToBeSelected = m_todoTreeView->indexBelow(selectedModelIndex());
    if (!indexToBeSelected.isValid())
        return m_todoTreeView->model()->index(0, 0);
    else
        return indexToBeSelected;
}

QModelIndex TodoOutputPane::previousModelIndex()
{
    QModelIndex indexToBeSelected = m_todoTreeView->indexAbove(selectedModelIndex());
    if (!indexToBeSelected.isValid())
        return m_todoTreeView->model()->index(m_todoTreeView->model()->rowCount() - 1, 0);
    else
        return indexToBeSelected;
}

} // namespace Internal
} // namespace Todo
