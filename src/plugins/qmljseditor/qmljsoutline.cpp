// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditor.h"
#include "qmljseditortr.h"
#include "qmljsoutline.h"
#include "qmljsoutlinetreeview.h"
#include "qmloutlinemodel.h"

#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/ioutlinewidget.h>

#include <QAction>
#include <QSortFilterProxyModel>
#include <QTextBlock>
#include <QVBoxLayout>

using namespace QmlJS;

enum {
    debug = false
};

namespace QmlJSEditor::Internal {

// QmlJSOutlineFilterModel

class QmlJSOutlineFilterModel final : public QSortFilterProxyModel
{
public:
    QmlJSOutlineFilterModel()
    {
        setDynamicSortFilter(true);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const final;
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const final;
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const final;
    QVariant data(const QModelIndex &index, int role) const final;
    Qt::DropActions supportedDragActions() const final;

    bool filterBindings() const;
    void setFilterBindings(bool filterBindings);
    void setSorted(bool sorted);

private:
    bool m_filterBindings = false;
    bool m_sorted = false;
};

Qt::ItemFlags QmlJSOutlineFilterModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags f = sourceModel()->flags(index);
    if (m_sorted)
        f.setFlag(Qt::ItemIsDropEnabled, false);
    return f;
}

bool QmlJSOutlineFilterModel::filterAcceptsRow(int sourceRow,
                                               const QModelIndex &sourceParent) const
{
    if (m_filterBindings) {
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        while (sourceIndex.isValid()) {
            if (sourceIndex.data(QmlOutlineModel::ItemTypeRole)
                == QmlOutlineModel::NonElementBindingType) {
                return false;
            }
            sourceIndex = sourceIndex.parent();
        }
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

bool QmlJSOutlineFilterModel::lessThan(const QModelIndex &sourceLeft,
                                       const QModelIndex &sourceRight) const
{
    if (!m_sorted)
        return sourceLeft.row() > sourceRight.row();

    return sourceLeft.data().toString() > sourceRight.data().toString();
}

QVariant QmlJSOutlineFilterModel::data(const QModelIndex &index, int role) const
{
    if (role == QmlOutlineModel::AnnotationRole) {
        // Don't show element id etc behind element if the property is also visible
        if (!filterBindings()
                && index.data(QmlOutlineModel::ItemTypeRole) == QmlOutlineModel::ElementType) {
            return QVariant();
        }
    }
    return QSortFilterProxyModel::data(index, role);
}

Qt::DropActions QmlJSOutlineFilterModel::supportedDragActions() const
{
    return sourceModel()->supportedDragActions();
}

bool QmlJSOutlineFilterModel::filterBindings() const
{
    return m_filterBindings;
}

void QmlJSOutlineFilterModel::setFilterBindings(bool filterBindings)
{
    m_filterBindings = filterBindings;
    invalidateFilter();
}

void QmlJSOutlineFilterModel::setSorted(bool sorted)
{
    m_sorted = sorted;
    invalidate();
}

// QmlJSOutlineWidget

class QmlJSOutlineWidget final : public TextEditor::IOutlineWidget
{
public:
    QmlJSOutlineWidget();

    void setEditor(QmlJSEditorWidget *editor);

    // IOutlineWidget
    QList<QAction*> filterMenuActions() const final;
    void setCursorSynchronization(bool syncWithCursor) final;
    bool isSorted() const final { return m_sorted; };
    void setSorted(bool sorted) final;
    void restoreSettings(const QVariantMap &map) final;
    QVariantMap settings() const final;

private:
    void updateSelectionInTree(const QModelIndex &index);
    void updateSelectionInText(const QItemSelection &selection);
    void updateTextCursor(const QModelIndex &index);
    void focusEditor();
    void setShowBindings(bool showBindings);
    bool syncCursor();

private:
    QmlJSOutlineTreeView *m_treeView = nullptr;
    QmlJSOutlineFilterModel m_filterModel;
    QmlJSEditorWidget *m_editor = nullptr;

    QAction *m_showBindingsAction = nullptr;

    bool m_enableCursorSync = true;
    bool m_blockCursorSync = false;
    bool m_sorted = false;
};

QmlJSOutlineWidget::QmlJSOutlineWidget()
    : m_treeView(new QmlJSOutlineTreeView(this))
{
    m_filterModel.setFilterBindings(false);

    m_treeView->setModel(&m_filterModel);
    m_treeView->setSortingEnabled(true);

    setFocusProxy(m_treeView);

    auto layout = new QVBoxLayout;

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_treeView));

    m_showBindingsAction = new QAction(this);
    m_showBindingsAction->setText(Tr::tr("Show All Bindings"));
    m_showBindingsAction->setCheckable(true);
    m_showBindingsAction->setChecked(true);
    connect(m_showBindingsAction, &QAction::toggled, this, &QmlJSOutlineWidget::setShowBindings);

    setLayout(layout);
}

void QmlJSOutlineWidget::setEditor(QmlJSEditorWidget *editor)
{
    m_editor = editor;

    m_filterModel.setSourceModel(m_editor->qmlJsEditorDocument()->outlineModel());
    m_treeView->expandAll();
    connect(m_editor->qmlJsEditorDocument()->outlineModel(), &QAbstractItemModel::modelAboutToBeReset, m_treeView, [this] {
        if (m_treeView->selectionModel())
            m_treeView->selectionModel()->blockSignals(true);
    });
    connect(m_editor->qmlJsEditorDocument()->outlineModel(), &QAbstractItemModel::modelReset, m_treeView, [this] {
        if (m_treeView->selectionModel())
            m_treeView->selectionModel()->blockSignals(false);
    });

    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &QmlJSOutlineWidget::updateSelectionInText);

    connect(m_treeView, &QAbstractItemView::activated,
            this, &QmlJSOutlineWidget::focusEditor);

    connect(m_editor, &QmlJSEditorWidget::outlineModelIndexChanged,
            this, &QmlJSOutlineWidget::updateSelectionInTree);
    connect(m_editor->qmlJsEditorDocument()->outlineModel(),
            &QmlOutlineModel::updated,
            this,
            [treeView = QPointer(m_treeView), editor = QPointer(m_editor)]() {
                if (treeView)
                    treeView->expandAll();
                if (editor)
                    editor->updateOutlineIndexNow();
            });
}

QList<QAction*> QmlJSOutlineWidget::filterMenuActions() const
{
    return {m_showBindingsAction};
}

void QmlJSOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    m_editor->updateOutlineIndexNow();
}

void QmlJSOutlineWidget::setSorted(bool sorted)
{
    m_sorted = sorted;
    m_filterModel.setSorted(m_sorted);
}

void QmlJSOutlineWidget::restoreSettings(const QVariantMap &map)
{
    bool showBindings = map.value(QString::fromLatin1("QmlJSOutline.ShowBindings"), true).toBool();
    m_showBindingsAction->setChecked(showBindings);
    setSorted(map.value(QString("QmlJSOutline.Sort"), false).toBool());
}

QVariantMap QmlJSOutlineWidget::settings() const
{
    return {
        {QString("QmlJSOutline.ShowBindings"), m_showBindingsAction->isChecked()},
        {QString("QmlJSOutline.Sort"), m_sorted}
    };
}

void QmlJSOutlineWidget::updateSelectionInTree(const QModelIndex &index)
{
    if (!syncCursor())
        return;

    m_blockCursorSync = true;

    QModelIndex baseIndex = index;
    QModelIndex filterIndex = m_filterModel.mapFromSource(baseIndex);
    while (baseIndex.isValid() && !filterIndex.isValid()) { // Search for ancestor index actually shown
        baseIndex = baseIndex.parent();
        filterIndex = m_filterModel.mapFromSource(baseIndex);
    }

    m_treeView->setCurrentIndex(filterIndex);
    m_treeView->scrollTo(filterIndex);
    m_blockCursorSync = false;
}

void QmlJSOutlineWidget::updateSelectionInText(const QItemSelection &selection)
{
    if (!syncCursor())
        return;

    if (!selection.indexes().isEmpty()) {
        QModelIndex index = selection.indexes().first();

        updateTextCursor(index);
    }
}

void QmlJSOutlineWidget::updateTextCursor(const QModelIndex &index)
{
    const auto update = [this](const QModelIndex &index) {
        if (!m_editor->isOutlineCursorChangesBlocked()) {
            QModelIndex sourceIndex = m_filterModel.mapToSource(index);

            SourceLocation location
                    = m_editor->qmlJsEditorDocument()->outlineModel()->sourceLocation(sourceIndex);

            if (!location.isValid())
                return;

            const QTextBlock lastBlock = m_editor->document()->lastBlock();
            const uint textLength = lastBlock.position() + lastBlock.length();
            if (location.offset >= textLength)
                return;

            Core::EditorManager::cutForwardNavigationHistory();
            Core::EditorManager::addCurrentPositionToNavigationHistory();

            QTextCursor textCursor = m_editor->textCursor();

            textCursor.setPosition(location.offset);
            m_editor->setTextCursor(textCursor);
            m_editor->centerCursor();
        }
    };
    m_blockCursorSync = true;
    update(index);
    m_blockCursorSync = false;
}

void QmlJSOutlineWidget::focusEditor()
{
    m_editor->setFocus();
}

void QmlJSOutlineWidget::setShowBindings(bool showBindings)
{
    m_filterModel.setFilterBindings(!showBindings);
    m_treeView->expandAll();
    m_editor->updateOutlineIndexNow();
}

bool QmlJSOutlineWidget::syncCursor()
{
    return m_enableCursorSync && !m_blockCursorSync;
}

class QmlJSOutlineWidgetFactory final : public TextEditor::IOutlineWidgetFactory
{
public:
    bool supportsEditor(Core::IEditor *editor) const final
    {
        return bool(qobject_cast<QmlJSEditor*>(editor));
    }

    bool supportsSorting() const final
    {
        return true;
    }

    TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor) final
    {
        auto widget = new QmlJSOutlineWidget;

        auto qmlJSEditable = qobject_cast<const QmlJSEditor*>(editor);
        auto qmlJSEditor = qobject_cast<QmlJSEditorWidget*>(qmlJSEditable->widget());
        Q_ASSERT(qmlJSEditor);

        widget->setEditor(qmlJSEditor);

        return widget;
    }
};

void setupQmlJsOutline()
{
    static QmlJSOutlineWidgetFactory theQmlJSOutlineWidgetFactory;
}

} // namespace QmlJSEditor::Internal
