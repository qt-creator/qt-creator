// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "workspaceview.h"

#include "dockmanager.h"
#include "advanceddockingsystemtr.h"

#include <utils/algorithm.h>

#include <QFileDialog>
#include <QHeaderView>
#include <QItemSelection>
#include <QStringList>
#include <QStyledItemDelegate>

namespace ADS {

// custom item delegate class
class RemoveItemFocusDelegate : public QStyledItemDelegate
{
public:
    RemoveItemFocusDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

protected:
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

void RemoveItemFocusDelegate::paint(QPainter *painter,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;
    QStyledItemDelegate::paint(painter, opt, index);
}

WorkspaceView::WorkspaceView(DockManager *manager, QWidget *parent)
    : Utils::TreeView(parent)
    , m_manager(manager)
    , m_workspaceModel(manager)
{
    setItemDelegate(new RemoveItemFocusDelegate(this));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setWordWrap(false);
    setRootIsDecorated(false);
    setSortingEnabled(true);

    setModel(&m_workspaceModel);
    sortByColumn(0, Qt::AscendingOrder);

    // Ensure that the full workspace name is visible.
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    QItemSelection firstRow(m_workspaceModel.index(0, 0),
                            m_workspaceModel.index(0, m_workspaceModel.columnCount() - 1));
    selectionModel()->select(firstRow, QItemSelectionModel::QItemSelectionModel::SelectCurrent);

    connect(this, &Utils::TreeView::activated, this, [this](const QModelIndex &index) {
        emit workspaceActivated(m_workspaceModel.workspaceAt(index.row()));
    });
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        emit workspacesSelected(selectedWorkspaces());
    });

    connect(&m_workspaceModel, &WorkspaceModel::workspaceSwitched,
            this, &WorkspaceView::workspaceSwitched);
    connect(&m_workspaceModel, &WorkspaceModel::modelReset,
            this, &WorkspaceView::selectActiveWorkspace);
    connect(&m_workspaceModel, &WorkspaceModel::workspaceCreated,
            this, &WorkspaceView::selectWorkspace);
}

void WorkspaceView::createNewWorkspace()
{
    m_workspaceModel.newWorkspace(this);
}

void WorkspaceView::deleteSelectedWorkspaces()
{
    deleteWorkspaces(selectedWorkspaces());
}

void WorkspaceView::deleteWorkspaces(const QStringList &workspaces)
{
    m_workspaceModel.deleteWorkspaces(workspaces);
}

void WorkspaceView::importWorkspace()
{
    static QString lastDir;
    const QString currentDir = lastDir.isEmpty() ? "" : lastDir;
    const auto fileName = QFileDialog::getOpenFileName(this,
                                                       Tr::tr("Import Workspace"),
                                                       currentDir,
                                                       "Workspaces (*" + m_manager->workspaceFileExtension() + ")");

    if (!fileName.isEmpty())
        lastDir = QFileInfo(fileName).absolutePath();

    m_workspaceModel.importWorkspace(fileName);
}

void WorkspaceView::exportCurrentWorkspace()
{
    static QString lastDir;
    const QString currentDir = lastDir.isEmpty() ? "" : lastDir;
    QFileInfo fileInfo(currentDir, m_manager->workspaceNameToFileName(currentWorkspace()));

    const auto fileName = QFileDialog::getSaveFileName(this,
                                                       Tr::tr("Export Workspace"),
                                                       fileInfo.absoluteFilePath(),
                                                       "Workspaces (*" + m_manager->workspaceFileExtension() + ")");

    if (!fileName.isEmpty())
        lastDir = QFileInfo(fileName).absolutePath();

    m_workspaceModel.exportWorkspace(fileName, currentWorkspace());
}

void WorkspaceView::cloneCurrentWorkspace()
{
    m_workspaceModel.cloneWorkspace(this, currentWorkspace());
}

void WorkspaceView::renameCurrentWorkspace()
{
    m_workspaceModel.renameWorkspace(this, currentWorkspace());
}

void WorkspaceView::resetCurrentWorkspace()
{
    m_workspaceModel.resetWorkspace(currentWorkspace());
}

void WorkspaceView::switchToCurrentWorkspace()
{
    m_workspaceModel.switchToWorkspace(currentWorkspace());
}

QString WorkspaceView::currentWorkspace()
{
    return m_workspaceModel.workspaceAt(selectionModel()->currentIndex().row());
}

WorkspaceModel *WorkspaceView::workspaceModel()
{
    return &m_workspaceModel;
}

void WorkspaceView::selectActiveWorkspace()
{
    selectWorkspace(m_manager->activeWorkspace());
}

void WorkspaceView::selectWorkspace(const QString &workspaceName)
{
    int row = m_workspaceModel.indexOfWorkspace(workspaceName);
    selectionModel()->setCurrentIndex(model()->index(row, 0),
                                      QItemSelectionModel::ClearAndSelect
                                          | QItemSelectionModel::Rows);
}

void WorkspaceView::showEvent(QShowEvent *event)
{
    Utils::TreeView::showEvent(event);
    selectActiveWorkspace();
    setFocus();
}

void WorkspaceView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() != Qt::Key_Delete && event->key() != Qt::Key_Backspace) {
        TreeView::keyPressEvent(event);
        return;
    }
    const QStringList workspaces = selectedWorkspaces();
    if (!Utils::anyOf(workspaces, [this](const QString &workspace) {
            return workspace == m_manager->activeWorkspace();
        })) {
        deleteWorkspaces(workspaces);
    }
}

QStringList WorkspaceView::selectedWorkspaces() const
{
    return Utils::transform(selectionModel()->selectedRows(), [this](const QModelIndex &index) {
        return m_workspaceModel.workspaceAt(index.row());
    });
}

} // namespace ADS
