// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "workspaceview.h"

#include "advanceddockingsystemtr.h"
#include "dockmanager.h"

#include <utils/algorithm.h>

#include <QFileDialog>
#include <QHeaderView>
#include <QItemSelection>
#include <QMessageBox>
#include <QMimeData>
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
    setUniformRowHeights(false);
    setItemDelegate(new RemoveItemFocusDelegate(this));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setWordWrap(false);
    setRootIsDecorated(false);
    setSortingEnabled(false);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);

    setModel(&m_workspaceModel);

    header()->setDefaultSectionSize(150);

    QItemSelection firstRow(m_workspaceModel.index(0, 0),
                            m_workspaceModel.index(0, m_workspaceModel.columnCount() - 1));
    selectionModel()->select(firstRow, QItemSelectionModel::QItemSelectionModel::SelectCurrent);

    connect(this, &Utils::TreeView::activated, this, [this](const QModelIndex &index) {
        emit workspaceActivated(m_workspaceModel.workspaceAt(index.row()));
    });
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        emit workspacesSelected(selectedWorkspaces());
    });

    connect(&m_workspaceModel,
            &WorkspaceModel::modelReset,
            this,
            &WorkspaceView::selectActiveWorkspace);
}

void WorkspaceView::createNewWorkspace()
{
    WorkspaceNameInputDialog workspaceInputDialog(m_manager, this);
    workspaceInputDialog.setWindowTitle(Tr::tr("New Workspace Name"));
    workspaceInputDialog.setActionText(Tr::tr("&Create"), Tr::tr("Create and &Open"));

    runWorkspaceNameInputDialog(&workspaceInputDialog, [this](const QString &newName) {
        Utils::expected_str<QString> result = m_manager->createWorkspace(newName);

        if (!result)
            QMessageBox::warning(this, Tr::tr("Cannot Create Workspace"), result.error());

        return result;
    });
}

void WorkspaceView::cloneCurrentWorkspace()
{
    const QString fileName = currentWorkspace();

    QString displayName = "Unknown";
    Workspace *workspace = m_manager->workspace(fileName);
    if (workspace)
        displayName = workspace->name();

    WorkspaceNameInputDialog workspaceInputDialog(m_manager, this);
    workspaceInputDialog.setWindowTitle(Tr::tr("New Workspace Name"));
    workspaceInputDialog.setActionText(Tr::tr("&Clone"), Tr::tr("Clone and &Open"));
    workspaceInputDialog.setValue(Tr::tr("%1 Copy").arg(displayName));

    runWorkspaceNameInputDialog(&workspaceInputDialog, [this, fileName](const QString &newName) {
        Utils::expected_str<QString> result = m_manager->cloneWorkspace(fileName, newName);

        if (!result)
            QMessageBox::warning(this, Tr::tr("Cannot Clone Workspace"), result.error());

        return result;
    });
}

void WorkspaceView::renameCurrentWorkspace()
{
    const QString fileName = currentWorkspace();

    QString displayName = "Unknown";
    Workspace *workspace = m_manager->workspace(fileName);
    if (workspace)
        displayName = workspace->name();

    WorkspaceNameInputDialog workspaceInputDialog(m_manager, this);
    workspaceInputDialog.setWindowTitle(Tr::tr("Rename Workspace"));
    workspaceInputDialog.setActionText(Tr::tr("&Rename"), Tr::tr("Rename and &Open"));
    workspaceInputDialog.setValue(displayName);

    runWorkspaceNameInputDialog(&workspaceInputDialog, [this, fileName](const QString &newName) {
        Utils::expected_str<QString> result = m_manager->renameWorkspace(fileName, newName);

        if (!result)
            QMessageBox::warning(this, Tr::tr("Cannot Rename Workspace"), result.error());

        return result;
    });
}

void WorkspaceView::resetCurrentWorkspace()
{
    const QString fileName = currentWorkspace();

    if (m_manager->resetWorkspacePreset(fileName) && fileName == *m_manager->activeWorkspace()) {
        if (m_manager->reloadActiveWorkspace())
            m_workspaceModel.resetWorkspaces();
    }
}

void WorkspaceView::switchToCurrentWorkspace()
{
    Utils::expected_str<void> result = m_manager->openWorkspace(currentWorkspace());

    if (!result)
        QMessageBox::warning(this, Tr::tr("Cannot Switch Workspace"), result.error());

    emit workspaceSwitched();
}

void WorkspaceView::deleteSelectedWorkspaces()
{
    deleteWorkspaces(selectedWorkspaces());
}

void WorkspaceView::importWorkspace()
{
    static QString previousDirectory;
    const QString currentDirectory = previousDirectory.isEmpty() ? "" : previousDirectory;
    const auto filePath
        = QFileDialog::getOpenFileName(this,
                                       Tr::tr("Import Workspace"),
                                       currentDirectory,
                                       QString("Workspaces (*.%1)").arg(workspaceFileExtension));

    // If the user presses Cancel, it returns a null string
    if (filePath.isEmpty())
        return;

    previousDirectory = QFileInfo(filePath).absolutePath();

    const Utils::expected_str<QString> newFileName = m_manager->importWorkspace(filePath);
    if (newFileName)
        m_workspaceModel.resetWorkspaces();
    else
        QMessageBox::warning(this, Tr::tr("Cannot Import Workspace"), newFileName.error());
}

void WorkspaceView::exportCurrentWorkspace()
{
    static QString previousDirectory;
    const QString currentDirectory = previousDirectory.isEmpty() ? "" : previousDirectory;
    QFileInfo fileInfo(currentDirectory, currentWorkspace());

    const auto filePath
        = QFileDialog::getSaveFileName(this,
                                       Tr::tr("Export Workspace"),
                                       fileInfo.absoluteFilePath(),
                                       QString("Workspaces (*.%1)").arg(workspaceFileExtension));

    // If the user presses Cancel, it returns a null string
    if (filePath.isEmpty())
        return;

    previousDirectory = QFileInfo(filePath).absolutePath();

    const Utils::expected_str<QString> result = m_manager->exportWorkspace(filePath,
                                                                           currentWorkspace());

    if (!result)
        QMessageBox::warning(this, Tr::tr("Cannot Export Workspace"), result.error());
}

void WorkspaceView::moveWorkspaceUp()
{
    const QString w = currentWorkspace();
    bool hasMoved = m_manager->moveWorkspaceUp(w);
    if (hasMoved) {
        m_workspaceModel.resetWorkspaces();
        selectWorkspace(w);
    }
}

void WorkspaceView::moveWorkspaceDown()
{
    const QString w = currentWorkspace();
    bool hasMoved = m_manager->moveWorkspaceDown(w);
    if (hasMoved) {
        m_workspaceModel.resetWorkspaces();
        selectWorkspace(w);
    }
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
    selectWorkspace(m_manager->activeWorkspace()->fileName());
}

void WorkspaceView::selectWorkspace(const QString &fileName)
{
    int row = m_workspaceModel.indexOfWorkspace(fileName);
    selectionModel()->setCurrentIndex(model()->index(row, 0),
                                      QItemSelectionModel::ClearAndSelect
                                          | QItemSelectionModel::Rows);
}

QStringList WorkspaceView::selectedWorkspaces() const
{
    return Utils::transform(selectionModel()->selectedRows(), [this](const QModelIndex &index) {
        return m_workspaceModel.workspaceAt(index.row());
    });
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
    const QStringList fileNames = selectedWorkspaces();
    if (!Utils::anyOf(fileNames, [this](const QString &fileName) {
            return fileName == *m_manager->activeWorkspace();
        })) {
        deleteWorkspaces(fileNames);
    }
}

void WorkspaceView::dropEvent(QDropEvent *event)
{
    const QModelIndex dropIndex = indexAt(event->pos());
    const DropIndicatorPosition dropIndicator = dropIndicatorPosition();

    const auto droppedWorkspaces = selectedWorkspaces();
    int from = m_manager->workspaceIndex(droppedWorkspaces.first());
    int to = dropIndex.row();

    if (dropIndicator == QAbstractItemView::AboveItem && from < to)
        --to;
    if (dropIndicator == QAbstractItemView::BelowItem && from > to)
        ++to;

    bool hasMoved = m_manager->moveWorkspace(from, to);

    if (hasMoved) {
        m_workspaceModel.resetWorkspaces();
        selectionModel()->setCurrentIndex(model()->index(to, 0),
                                          QItemSelectionModel::ClearAndSelect
                                              | QItemSelectionModel::Rows);
    }

    event->acceptProposedAction();
}

void WorkspaceView::deleteWorkspaces(const QStringList &fileNames)
{
    if (!confirmWorkspaceDelete(fileNames))
        return;

    m_manager->deleteWorkspaces(fileNames);
    m_workspaceModel.resetWorkspaces();
}

bool WorkspaceView::confirmWorkspaceDelete(const QStringList &fileNames)
{
    const QString title = fileNames.size() == 1 ? Tr::tr("Delete Workspace")
                                                : Tr::tr("Delete Workspaces");
    const QString question = fileNames.size() == 1
                                 ? Tr::tr("Delete workspace \"%1\"?").arg(fileNames.first())
                                 : Tr::tr("Delete these workspaces?")
                                       + QString("\n    %1").arg(fileNames.join("\n    "));
    return QMessageBox::question(parentWidget(), title, question, QMessageBox::Yes | QMessageBox::No)
           == QMessageBox::Yes;
}

void WorkspaceView::runWorkspaceNameInputDialog(
    WorkspaceNameInputDialog *workspaceInputDialog,
    std::function<Utils::expected_str<QString>(const QString &)> callback)
{
    if (workspaceInputDialog->exec() == QDialog::Accepted) {
        const QString newWorkspace = workspaceInputDialog->value();
        if (newWorkspace.isEmpty() || m_manager->workspaces().contains(newWorkspace))
            return;

        const Utils::expected_str<QString> fileName = callback(newWorkspace);
        if (!fileName)
            return;

        m_workspaceModel.resetWorkspaces();

        if (workspaceInputDialog->isSwitchToRequested()) {
            m_manager->openWorkspace(*fileName);
            emit workspaceSwitched();
        }

        selectWorkspace(*fileName);
    }
}

} // namespace ADS
