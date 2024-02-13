// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppoutline.h"

#include "cppeditordocument.h"
#include "cppeditoroutline.h"
#include "cppeditortr.h"
#include "cppeditorwidget.h"
#include "cppmodelmanager.h"
#include "cppoutlinemodel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/itemviewfind.h>

#include <texteditor/ioutlinewidget.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>

#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QVBoxLayout>

using namespace TextEditor;

namespace CppEditor::Internal {

class CppOutlineTreeView final : public Utils::NavigationTreeView
{
public:
    CppOutlineTreeView(QWidget *parent) :
        Utils::NavigationTreeView(parent)
    {
        setExpandsOnDoubleClick(false);
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragOnly);
    }

    void contextMenuEvent(QContextMenuEvent *event) final
    {
        if (!event)
            return;

        QMenu contextMenu;

        QAction *action = contextMenu.addAction(Tr::tr("Expand All"));
        connect(action, &QAction::triggered, this, &QTreeView::expandAll);
        action = contextMenu.addAction(Tr::tr("Collapse All"));
        connect(action, &QAction::triggered, this, &QTreeView::collapseAll);

        contextMenu.exec(event->globalPos());

        event->accept();
    }
};

class CppOutlineFilterModel : public QSortFilterProxyModel
{
public:
    CppOutlineFilterModel(OutlineModel &sourceModel, QObject *parent)
        : QSortFilterProxyModel(parent)
        , m_sourceModel(sourceModel)
    {}

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const final
    {
        // ignore artificial "<Select Symbol>" entry
        if (!sourceParent.isValid() && sourceRow == 0)
            return false;
        // ignore generated symbols, e.g. by macro expansion (Q_OBJECT)
        const QModelIndex sourceIndex = m_sourceModel.index(sourceRow, 0, sourceParent);
        if (m_sourceModel.isGenerated(sourceIndex))
            return false;

        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

    Qt::DropActions supportedDragActions() const final
    {
        return sourceModel()->supportedDragActions();
    }

private:
    OutlineModel &m_sourceModel;
};

class CppOutlineWidget : public TextEditor::IOutlineWidget
{
public:
    CppOutlineWidget(CppEditorWidget *editor);

    // IOutlineWidget
    QList<QAction*> filterMenuActions() const final;
    void setCursorSynchronization(bool syncWithCursor) final;
    bool isSorted() const final;
    void setSorted(bool sorted) final;

    void restoreSettings(const QVariantMap &map) final;
    QVariantMap settings() const final;

private:
    void modelUpdated();
    void updateIndex();
    void updateIndexNow();
    void updateTextCursor(const QModelIndex &index);
    void onItemActivated(const QModelIndex &index);
    bool syncCursor();

    CppEditorWidget *m_editor;
    CppOutlineTreeView *m_treeView;
    OutlineModel * const m_model;
    QSortFilterProxyModel *m_proxyModel;
    QTimer m_updateIndexTimer;

    bool m_enableCursorSync;
    bool m_blockCursorSync;
    bool m_sorted;
};

CppOutlineWidget::CppOutlineWidget(CppEditorWidget *editor) :
    m_editor(editor),
    m_treeView(new CppOutlineTreeView(this)),
    m_model(&m_editor->cppEditorDocument()->outlineModel()),
    m_proxyModel(new CppOutlineFilterModel(*m_model, this)),
    m_enableCursorSync(true),
    m_blockCursorSync(false),
    m_sorted(false)
{
    m_proxyModel->setSourceModel(m_model);

    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_treeView));
    setLayout(layout);

    m_treeView->setModel(m_proxyModel);
    m_treeView->setSortingEnabled(true);
    setFocusProxy(m_treeView);

    connect(m_model, &QAbstractItemModel::modelReset, this, &CppOutlineWidget::modelUpdated);
    modelUpdated();

    connect(m_treeView, &QAbstractItemView::activated,
            this, &CppOutlineWidget::onItemActivated);
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, [this] {
        if (m_model->rootItem()->hasChildren())
            updateIndex();
    });

    m_updateIndexTimer.setSingleShot(true);
    m_updateIndexTimer.setInterval(500);
    connect(&m_updateIndexTimer, &QTimer::timeout, this, &CppOutlineWidget::updateIndexNow);
}

QList<QAction*> CppOutlineWidget::filterMenuActions() const
{
    return {};
}

void CppOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateIndexNow();
}

bool CppOutlineWidget::isSorted() const
{
    return m_sorted;
}

void CppOutlineWidget::setSorted(bool sorted)
{
    m_sorted = sorted;
    m_proxyModel->sort(m_sorted ? 0 : -1);
}

void CppOutlineWidget::restoreSettings(const QVariantMap &map)
{
    setSorted(map.value(QString("CppOutline.Sort"), false).toBool());
}

QVariantMap CppOutlineWidget::settings() const
{
    return {{QString("CppOutline.Sort"), m_sorted}};
}

void CppOutlineWidget::modelUpdated()
{
    m_treeView->expandAll();
}

void CppOutlineWidget::updateIndex()
{
    m_updateIndexTimer.start();
}

void CppOutlineWidget::updateIndexNow()
{
    if (!syncCursor())
        return;

    const int revision = m_editor->document()->revision();
    if (m_model->editorRevision() != revision) {
        m_editor->cppEditorDocument()->updateOutline();
        return;
    }

    m_updateIndexTimer.stop();

    int line = 0, column = 0;
    m_editor->convertPosition(m_editor->position(), &line, &column);
    QModelIndex index = m_model->indexForPosition(line, column);

    if (index.isValid()) {
        m_blockCursorSync = true;
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
        m_treeView->setCurrentIndex(proxyIndex);
        m_treeView->scrollTo(proxyIndex);
        m_blockCursorSync = false;
    }

}

void CppOutlineWidget::updateTextCursor(const QModelIndex &proxyIndex)
{
    QModelIndex index = m_proxyModel->mapToSource(proxyIndex);
    Utils::Text::Position lineColumn
        = m_editor->cppEditorDocument()->outlineModel().positionFromIndex(index);
    if (!lineColumn.isValid())
        return;

    m_blockCursorSync = true;

    Core::EditorManager::cutForwardNavigationHistory();
    Core::EditorManager::addCurrentPositionToNavigationHistory();

    m_editor->gotoLine(lineColumn.line, lineColumn.column, true, true);
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

class CppOutlineWidgetFactory final : public IOutlineWidgetFactory
{
public:
    bool supportsEditor(Core::IEditor *editor) const final
    {
        const auto cppEditor = qobject_cast<BaseTextEditor*>(editor);
        if (!cppEditor || !CppModelManager::isCppEditor(cppEditor))
            return false;
        return !CppModelManager::usesClangd(cppEditor->textDocument());
    }

    bool supportsSorting() const final
    {
        return true;
    }

    IOutlineWidget *createWidget(Core::IEditor *editor) final
    {
        const auto cppEditor = qobject_cast<BaseTextEditor*>(editor);
        QTC_ASSERT(cppEditor, return nullptr);
        const auto cppEditorWidget = qobject_cast<CppEditorWidget*>(cppEditor->widget());
        QTC_ASSERT(cppEditorWidget, return nullptr);

        return new CppOutlineWidget(cppEditorWidget);
    }
};

void setupCppOutline()
{
    static CppOutlineWidgetFactory theCppOutlineWidgetFactory;
}

} // namespace CppEditor::Internal
