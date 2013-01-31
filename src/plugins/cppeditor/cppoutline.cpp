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

#include "cppoutline.h"

#include <TranslationUnit.h>
#include <Symbol.h>

#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cplusplus/OverviewModel.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QTimer>
#include <QVBoxLayout>
#include <QMenu>

using namespace CppEditor::Internal;

enum {
    debug = false
};

CppOutlineTreeView::CppOutlineTreeView(QWidget *parent) :
    Utils::NavigationTreeView(parent)
{
    // see also QmlJSOutlineTreeView
    setFocusPolicy(Qt::NoFocus);
    setExpandsOnDoubleClick(false);
}

void CppOutlineTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!event)
        return;

    QMenu contextMenu;

    contextMenu.addAction(tr("Expand All"), this, SLOT(expandAll()));
    contextMenu.addAction(tr("Collapse All"), this, SLOT(collapseAll()));

    contextMenu.exec(event->globalPos());

    event->accept();
}

CppOutlineFilterModel::CppOutlineFilterModel(CPlusPlus::OverviewModel *sourceModel, QObject *parent) :
    QSortFilterProxyModel(parent),
    m_sourceModel(sourceModel)
{
    setSourceModel(m_sourceModel);
}

bool CppOutlineFilterModel::filterAcceptsRow(int sourceRow,
                                             const QModelIndex &sourceParent) const
{
    // ignore artifical "<Select Symbol>" entry
    if (!sourceParent.isValid() && sourceRow == 0)
        return false;
    // ignore generated symbols, e.g. by macro expansion (Q_OBJECT)
    const QModelIndex sourceIndex = m_sourceModel->index(sourceRow, 0, sourceParent);
    CPlusPlus::Symbol *symbol = m_sourceModel->symbolFromIndex(sourceIndex);
    if (symbol && symbol->isGenerated())
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}


CppOutlineWidget::CppOutlineWidget(CPPEditorWidget *editor) :
    TextEditor::IOutlineWidget(),
    m_editor(editor),
    m_treeView(new CppOutlineTreeView(this)),
    m_model(m_editor->outlineModel()),
    m_proxyModel(new CppOutlineFilterModel(m_model, this)),
    m_enableCursorSync(true),
    m_blockCursorSync(false)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_treeView);
    setLayout(layout);

    m_treeView->setModel(m_proxyModel);

    connect(m_model, SIGNAL(modelReset()), this, SLOT(modelUpdated()));
    modelUpdated();

    connect(m_editor, SIGNAL(outlineModelIndexChanged(QModelIndex)),
            this, SLOT(updateSelectionInTree(QModelIndex)));
    connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateSelectionInText(QItemSelection)));
    connect(m_treeView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(updateTextCursor(QModelIndex)));
}

QList<QAction*> CppOutlineWidget::filterMenuActions() const
{
    return QList<QAction*>();
}

void CppOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateSelectionInTree(m_editor->outlineModelIndex());
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
    if (debug)
        qDebug() << "CppOutline - updating selection due to cursor move";

    m_treeView->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
    m_treeView->scrollTo(proxyIndex);
    m_blockCursorSync = false;
}

void CppOutlineWidget::updateSelectionInText(const QItemSelection &selection)
{
    if (!syncCursor())
        return;

    if (!selection.indexes().isEmpty()) {
        QModelIndex proxyIndex = selection.indexes().first();
        updateTextCursor(proxyIndex);
    }
}

void CppOutlineWidget::updateTextCursor(const QModelIndex &proxyIndex)
{
    QModelIndex index = m_proxyModel->mapToSource(proxyIndex);
    CPlusPlus::Symbol *symbol = m_model->symbolFromIndex(index);
    if (symbol) {
        m_blockCursorSync = true;

        if (debug)
            qDebug() << "CppOutline - moving cursor to" << symbol->line() << symbol->column() - 1;

        Core::EditorManager *editorManager = Core::EditorManager::instance();
        editorManager->cutForwardNavigationHistory();
        editorManager->addCurrentPositionToNavigationHistory();

        // line has to be 1 based, column 0 based!
        m_editor->gotoLine(symbol->line(), symbol->column() - 1);
        m_editor->setFocus();
        m_blockCursorSync = false;
    }
}

bool CppOutlineWidget::syncCursor()
{
    return m_enableCursorSync && !m_blockCursorSync;
}

bool CppOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    if (qobject_cast<CPPEditor*>(editor))
        return true;
    return false;
}

TextEditor::IOutlineWidget *CppOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    CPPEditor *cppEditor = qobject_cast<CPPEditor*>(editor);
    CPPEditorWidget *cppEditorWidget = qobject_cast<CPPEditorWidget*>(cppEditor->widget());
    QTC_ASSERT(cppEditorWidget, return 0);

    CppOutlineWidget *widget = new CppOutlineWidget(cppEditorWidget);

    return widget;
}
