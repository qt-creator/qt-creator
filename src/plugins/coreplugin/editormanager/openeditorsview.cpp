// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openeditorsview.h"

#include "documentmodel.h"
#include "editormanager.h"
#include "ieditor.h"
#include "../actionmanager/command.h"
#include "../coreplugintr.h"
#include "../opendocumentstreeview.h"

#include <utils/fsengine/fileiconprovider.h>
#include <utils/qtcassert.h>

#include <QAbstractProxyModel>
#include <QApplication>
#include <QMenu>

using namespace Utils;

namespace Core::Internal {

class ProxyModel : public QAbstractProxyModel
{
public:
    explicit ProxyModel(QObject *parent = nullptr);

    QModelIndex mapFromSource(const QModelIndex & sourceIndex) const override;
    QModelIndex mapToSource(const QModelIndex & proxyIndex) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QVariant data(const QModelIndex &index, int role) const override;

    // QAbstractProxyModel::sibling is broken in Qt 5
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;
    // QAbstractProxyModel::supportedDragActions delegation is missing in Qt 5
    Qt::DropActions supportedDragActions() const override;

private:
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void sourceRowsRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
};

// OpenEditorsWidget

class OpenEditorsWidget : public OpenDocumentsTreeView
{
public:
    OpenEditorsWidget();
    ~OpenEditorsWidget() override;

private:
    void handleActivated(const QModelIndex &);
    void updateCurrentItem(IEditor*);
    void contextMenuRequested(QPoint pos);
    void activateEditor(const QModelIndex &index);
    void closeDocument(const QModelIndex &index);

    ProxyModel *m_model;
};

OpenEditorsWidget::OpenEditorsWidget()
{
    setWindowTitle(Tr::tr("Open Documents"));
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    m_model = new ProxyModel(this);
    m_model->setSourceModel(DocumentModel::model());
    setModel(m_model);

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &OpenEditorsWidget::updateCurrentItem);
    connect(this, &OpenDocumentsTreeView::activated,
            this, &OpenEditorsWidget::handleActivated);
    connect(this, &OpenDocumentsTreeView::closeActivated,
            this, &OpenEditorsWidget::closeDocument);

    connect(this, &OpenDocumentsTreeView::customContextMenuRequested,
            this, &OpenEditorsWidget::contextMenuRequested);
    updateCurrentItem(EditorManager::currentEditor());
}

OpenEditorsWidget::~OpenEditorsWidget() = default;

void OpenEditorsWidget::updateCurrentItem(IEditor *editor)
{
    if (!editor) {
        clearSelection();
        return;
    }
    const std::optional<int> index = DocumentModel::indexOfDocument(editor->document());
    if (QTC_GUARD(index))
        setCurrentIndex(m_model->index(index.value(), 0));
    selectionModel()->select(currentIndex(),
                             QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    scrollTo(currentIndex());
}

void OpenEditorsWidget::handleActivated(const QModelIndex &index)
{
    if (index.column() == 0) {
        activateEditor(index);
    } else if (index.column() == 1) { // the funky close button
        closeDocument(index);

        // work around a bug in itemviews where the delegate wouldn't get the QStyle::State_MouseOver
        QPoint cursorPos = QCursor::pos();
        QWidget *vp = viewport();
        QMouseEvent e(QEvent::MouseMove, vp->mapFromGlobal(cursorPos), cursorPos, Qt::NoButton, {}, {});
        QCoreApplication::sendEvent(vp, &e);
    }
}

void OpenEditorsWidget::activateEditor(const QModelIndex &index)
{
    selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    EditorManager::activateEditorForEntry(
                DocumentModel::entryAtRow(m_model->mapToSource(index).row()));
}

void OpenEditorsWidget::closeDocument(const QModelIndex &index)
{
    EditorManager::closeDocuments({DocumentModel::entryAtRow(m_model->mapToSource(index).row())});
    // work around selection changes
    updateCurrentItem(EditorManager::currentEditor());
}

void OpenEditorsWidget::contextMenuRequested(QPoint pos)
{
    QMenu contextMenu;
    QModelIndex editorIndex = indexAt(pos);
    const int row = m_model->mapToSource(editorIndex).row();
    DocumentModel::Entry *entry = DocumentModel::entryAtRow(row);
    EditorManager::addSaveAndCloseEditorActions(&contextMenu, entry);
    contextMenu.addSeparator();
    EditorManager::addPinEditorActions(&contextMenu, entry);
    contextMenu.addSeparator();
    EditorManager::addNativeDirAndOpenWithActions(&contextMenu, entry);
    contextMenu.exec(mapToGlobal(pos));
}

///
// OpenEditorsViewFactory
///

OpenEditorsViewFactory::OpenEditorsViewFactory()
{
    setId("Open Documents");
    setDisplayName(Tr::tr("Open Documents"));
    setActivationSequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+O") : Tr::tr("Alt+O")));
    setPriority(200);
}

NavigationView OpenEditorsViewFactory::createWidget()
{
    return {new OpenEditorsWidget, {}};
}

ProxyModel::ProxyModel(QObject *parent) : QAbstractProxyModel(parent)
{
}

QModelIndex ProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    // root
    if (!sourceIndex.isValid())
        return QModelIndex();
    // hide the <no document>
    int row = sourceIndex.row() - 1;
    if (row < 0)
        return QModelIndex();
    return createIndex(row, sourceIndex.column());
}

QModelIndex ProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
        return QModelIndex();
    // handle missing <no document>
    return sourceModel()->index(proxyIndex.row() + 1, proxyIndex.column());
}

QModelIndex ProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= sourceModel()->rowCount(mapToSource(parent)) - 1
            || column < 0 || column > 1)
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex ProxyModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int ProxyModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return sourceModel()->rowCount(mapToSource(parent)) - 1;
    return 0;
}

int ProxyModel::columnCount(const QModelIndex &parent) const
{
    return sourceModel()->columnCount(mapToSource(parent));
}

void ProxyModel::setSourceModel(QAbstractItemModel *sm)
{
    QAbstractItemModel *previousModel = sourceModel();
    if (previousModel) {
        disconnect(previousModel, &QAbstractItemModel::dataChanged,
                   this, &ProxyModel::sourceDataChanged);
        disconnect(previousModel, &QAbstractItemModel::rowsInserted,
                   this, &ProxyModel::sourceRowsInserted);
        disconnect(previousModel, &QAbstractItemModel::rowsRemoved,
                   this, &ProxyModel::sourceRowsRemoved);
        disconnect(previousModel, &QAbstractItemModel::rowsAboutToBeInserted,
                   this, &ProxyModel::sourceRowsAboutToBeInserted);
        disconnect(previousModel, &QAbstractItemModel::rowsAboutToBeRemoved,
                   this, &ProxyModel::sourceRowsAboutToBeRemoved);
    }
    QAbstractProxyModel::setSourceModel(sm);
    if (sm) {
        connect(sm, &QAbstractItemModel::dataChanged,
                this, &ProxyModel::sourceDataChanged);
        connect(sm, &QAbstractItemModel::rowsInserted,
                this, &ProxyModel::sourceRowsInserted);
        connect(sm, &QAbstractItemModel::rowsRemoved,
                this, &ProxyModel::sourceRowsRemoved);
        connect(sm, &QAbstractItemModel::rowsAboutToBeInserted,
                this, &ProxyModel::sourceRowsAboutToBeInserted);
        connect(sm, &QAbstractItemModel::rowsAboutToBeRemoved,
                this, &ProxyModel::sourceRowsAboutToBeRemoved);
    }
}

QVariant ProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole && index.column() == 0) {
        const QVariant sourceDecoration = QAbstractProxyModel::data(index, role);
        if (sourceDecoration.isValid())
            return sourceDecoration;
        const QVariant filePath = QAbstractProxyModel::data(index, DocumentModel::FilePathRole);
        return FileIconProvider::icon(FilePath::fromVariant(filePath));
    }

    return QAbstractProxyModel::data(index, role);
}

QModelIndex ProxyModel::sibling(int row, int column, const QModelIndex &idx) const
{
    return QAbstractItemModel::sibling(row, column, idx);
}

Qt::DropActions ProxyModel::supportedDragActions() const
{
    return sourceModel()->supportedDragActions();
}

void ProxyModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex topLeftIndex = mapFromSource(topLeft);
    if (!topLeftIndex.isValid())
        topLeftIndex = index(0, topLeft.column());
    QModelIndex bottomRightIndex = mapFromSource(bottomRight);
    if (!bottomRightIndex.isValid())
        bottomRightIndex = index(0, bottomRight.column());
    emit dataChanged(topLeftIndex, bottomRightIndex);
}

void ProxyModel::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    endRemoveRows();
}

void ProxyModel::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    endInsertRows();
}

void ProxyModel::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    int realStart = parent.isValid() || start == 0 ? start : start - 1;
    int realEnd = parent.isValid() || end == 0 ? end : end - 1;
    beginRemoveRows(parent, realStart, realEnd);
}

void ProxyModel::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    int realStart = parent.isValid() || start == 0 ? start : start - 1;
    int realEnd = parent.isValid() || end == 0 ? end : end - 1;
    beginInsertRows(parent, realStart, realEnd);
}

} // Core::Internal
