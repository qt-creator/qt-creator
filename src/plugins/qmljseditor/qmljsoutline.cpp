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

#include "qmljsoutline.h"
#include "qmloutlinemodel.h"
#include "qmljseditor.h"
#include "qmljsoutlinetreeview.h"

#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QSettings>
#include <QAction>
#include <QVBoxLayout>
#include <QTextBlock>

using namespace QmlJS;

enum {
    debug = false
};

namespace QmlJSEditor {
namespace Internal {


QmlJSOutlineFilterModel::QmlJSOutlineFilterModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

bool QmlJSOutlineFilterModel::filterAcceptsRow(int sourceRow,
                                               const QModelIndex &sourceParent) const
{
    if (m_filterBindings) {
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        QVariant itemType = sourceIndex.data(QmlOutlineModel::ItemTypeRole);
        if (itemType == QmlOutlineModel::NonElementBindingType)
            return false;
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
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

QmlJSOutlineWidget::QmlJSOutlineWidget(QWidget *parent)
    : TextEditor::IOutlineWidget(parent)
    , m_treeView(new QmlJSOutlineTreeView(this))
    , m_filterModel(new QmlJSOutlineFilterModel(this))
{
    m_filterModel->setFilterBindings(false);

    m_treeView->setModel(m_filterModel);
    setFocusProxy(m_treeView);

    QVBoxLayout *layout = new QVBoxLayout;

    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_treeView));

    m_showBindingsAction = new QAction(this);
    m_showBindingsAction->setText(tr("Show All Bindings"));
    m_showBindingsAction->setCheckable(true);
    m_showBindingsAction->setChecked(true);
    connect(m_showBindingsAction, &QAction::toggled, this, &QmlJSOutlineWidget::setShowBindings);

    setLayout(layout);
}

void QmlJSOutlineWidget::setEditor(QmlJSEditorWidget *editor)
{
    m_editor = editor;

    m_filterModel->setSourceModel(m_editor->qmlJsEditorDocument()->outlineModel());
    m_treeView->expandAll();
    connect(m_editor->qmlJsEditorDocument()->outlineModel(), &QAbstractItemModel::modelAboutToBeReset, m_treeView, [this]() {
        if (m_treeView->selectionModel())
            m_treeView->selectionModel()->blockSignals(true);
    });
    connect(m_editor->qmlJsEditorDocument()->outlineModel(), &QAbstractItemModel::modelReset, m_treeView, [this]() {
        if (m_treeView->selectionModel())
            m_treeView->selectionModel()->blockSignals(false);
    });

    connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &QmlJSOutlineWidget::updateSelectionInText);

    connect(m_treeView, &QAbstractItemView::activated,
            this, &QmlJSOutlineWidget::focusEditor);

    connect(m_editor, &QmlJSEditorWidget::outlineModelIndexChanged,
            this, &QmlJSOutlineWidget::updateSelectionInTree);
    connect(m_editor->qmlJsEditorDocument()->outlineModel(), &QmlOutlineModel::updated, this, [this] () {
        m_treeView->expandAll();
        m_editor->updateOutlineIndexNow();
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

void QmlJSOutlineWidget::restoreSettings(const QVariantMap &map)
{
    bool showBindings = map.value(QString::fromLatin1("QmlJSOutline.ShowBindings"), true).toBool();
    m_showBindingsAction->setChecked(showBindings);
}

QVariantMap QmlJSOutlineWidget::settings() const
{
    return {{QLatin1String("QmlJSOutline.ShowBindings"), m_showBindingsAction->isChecked()}};
}

void QmlJSOutlineWidget::updateSelectionInTree(const QModelIndex &index)
{
    if (!syncCursor())
        return;

    m_blockCursorSync = true;

    QModelIndex baseIndex = index;
    QModelIndex filterIndex = m_filterModel->mapFromSource(baseIndex);
    while (baseIndex.isValid() && !filterIndex.isValid()) { // Search for ancestor index actually shown
        baseIndex = baseIndex.parent();
        filterIndex = m_filterModel->mapFromSource(baseIndex);
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
            QModelIndex sourceIndex = m_filterModel->mapToSource(index);

            AST::SourceLocation location
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
    m_filterModel->setFilterBindings(!showBindings);
    m_treeView->expandAll();
    m_editor->updateOutlineIndexNow();
}

bool QmlJSOutlineWidget::syncCursor()
{
    return m_enableCursorSync && !m_blockCursorSync;
}

bool QmlJSOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    if (qobject_cast<QmlJSEditor*>(editor))
        return true;
    return false;
}

TextEditor::IOutlineWidget *QmlJSOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    QmlJSOutlineWidget *widget = new QmlJSOutlineWidget;

    QmlJSEditor *qmlJSEditable = qobject_cast<QmlJSEditor*>(editor);
    QmlJSEditorWidget *qmlJSEditor = qobject_cast<QmlJSEditorWidget*>(qmlJSEditable->widget());
    Q_ASSERT(qmlJSEditor);

    widget->setEditor(qmlJSEditor);

    return widget;
}

} // namespace Internal
} // namespace QmlJSEditor
