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

#include "cppoutline.h"

#include <cpptools/cppeditoroutline.h>

#include <cpptools/cppoverviewmodel.h>

#include <texteditor/textdocument.h>

#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/linecolumn.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QVBoxLayout>
#include <QMenu>

namespace CppEditor {
namespace Internal {

CppOutlineTreeView::CppOutlineTreeView(QWidget *parent) :
    Utils::NavigationTreeView(parent)
{
    setExpandsOnDoubleClick(false);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
}

void CppOutlineTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!event)
        return;

    QMenu contextMenu;

    QAction *action = contextMenu.addAction(tr("Expand All"));
    connect(action, &QAction::triggered, this, &QTreeView::expandAll);
    action = contextMenu.addAction(tr("Collapse All"));
    connect(action, &QAction::triggered, this, &QTreeView::collapseAll);

    contextMenu.exec(event->globalPos());

    event->accept();
}

CppOutlineFilterModel::CppOutlineFilterModel(CppTools::AbstractOverviewModel &sourceModel,
                                             QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_sourceModel(sourceModel)
{
}

bool CppOutlineFilterModel::filterAcceptsRow(int sourceRow,
                                             const QModelIndex &sourceParent) const
{
    // ignore artifical "<Select Symbol>" entry
    if (!sourceParent.isValid() && sourceRow == 0)
        return false;
    // ignore generated symbols, e.g. by macro expansion (Q_OBJECT)
    const QModelIndex sourceIndex = m_sourceModel.index(sourceRow, 0, sourceParent);
    if (m_sourceModel.isGenerated(sourceIndex))
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

Qt::DropActions CppOutlineFilterModel::supportedDragActions() const
{
    return sourceModel()->supportedDragActions();
}


CppOutlineWidget::CppOutlineWidget(CppEditorWidget *editor) :
    m_editor(editor),
    m_treeView(new CppOutlineTreeView(this)),
    m_enableCursorSync(true),
    m_blockCursorSync(false)
{
    CppTools::AbstractOverviewModel *model = m_editor->outline()->model();
    m_proxyModel = new CppOutlineFilterModel(*model, this);
    m_proxyModel->setSourceModel(model);

    auto *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_treeView));
    setLayout(layout);

    m_treeView->setModel(m_proxyModel);
    setFocusProxy(m_treeView);

    connect(model, &QAbstractItemModel::modelReset, this, &CppOutlineWidget::modelUpdated);
    modelUpdated();

    connect(m_editor->outline(), &CppTools::CppEditorOutline::modelIndexChanged,
            this, &CppOutlineWidget::updateSelectionInTree);
    connect(m_treeView, &QAbstractItemView::activated,
            this, &CppOutlineWidget::onItemActivated);
}

QList<QAction*> CppOutlineWidget::filterMenuActions() const
{
    return QList<QAction*>();
}

void CppOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateSelectionInTree(m_editor->outline()->modelIndex());
}

void CppOutlineWidget::modelUpdated()
{
    m_treeView->expandAll();
}

void CppOutlineWidget::updateSelectionInTree(const QModelIndex &index)
{
    if (!syncCursor())
        return;

    QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);

    m_blockCursorSync = true;
    m_treeView->setCurrentIndex(proxyIndex);
    m_treeView->scrollTo(proxyIndex);
    m_blockCursorSync = false;
}

void CppOutlineWidget::updateTextCursor(const QModelIndex &proxyIndex)
{
    QModelIndex index = m_proxyModel->mapToSource(proxyIndex);
    CppTools::AbstractOverviewModel *model = m_editor->outline()->model();
    Utils::LineColumn lineColumn = model->lineColumnFromIndex(index);
    if (!lineColumn.isValid())
        return;

    m_blockCursorSync = true;

    Core::EditorManager::cutForwardNavigationHistory();
    Core::EditorManager::addCurrentPositionToNavigationHistory();

    // line has to be 1 based, column 0 based!
    m_editor->gotoLine(lineColumn.line, lineColumn.column - 1, true, true);
    m_blockCursorSync = false;
}

void CppOutlineWidget::onItemActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    updateTextCursor(index);
    m_editor->setFocus();
}

bool CppOutlineWidget::syncCursor()
{
    return m_enableCursorSync && !m_blockCursorSync;
}

bool CppOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    return qobject_cast<CppEditor*>(editor);
}

TextEditor::IOutlineWidget *CppOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    auto *cppEditor = qobject_cast<CppEditor*>(editor);
    auto *cppEditorWidget = qobject_cast<CppEditorWidget*>(cppEditor->widget());
    QTC_ASSERT(cppEditorWidget, return nullptr);

    auto *widget = new CppOutlineWidget(cppEditorWidget);

    return widget;
}

} // namespace Internal
} // namespace CppEditor
