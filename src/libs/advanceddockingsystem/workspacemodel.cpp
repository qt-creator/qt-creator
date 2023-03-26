// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "workspacemodel.h"

#include "advanceddockingsystemtr.h"
#include "dockmanager.h"
#include "workspacedialog.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/stringutils.h>

#include <QFileInfo>

namespace ADS {

WorkspaceModel::WorkspaceModel(DockManager *manager, QObject *parent)
    : QAbstractTableModel(parent)
    , m_manager(manager)
    , m_currentSortColumn(0)
{
    m_sortedWorkspaces = m_manager->workspaces();
    sort(m_currentSortColumn, m_currentSortOrder);
    connect(m_manager, &DockManager::workspaceLoaded, this, &WorkspaceModel::resetWorkspaces);
}

int WorkspaceModel::indexOfWorkspace(const QString &workspace)
{
    return m_sortedWorkspaces.indexOf(workspace);
}

QString WorkspaceModel::workspaceAt(int row) const
{
    return m_sortedWorkspaces.value(row, QString());
}

QVariant WorkspaceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant result;
    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                result = Tr::tr("Workspace");
                break;
            case 1:
                result = Tr::tr("Last Modified");
                break;
            } // switch (section)
            break;
        } // switch (role)
    }
    return result;
}

int WorkspaceModel::columnCount(const QModelIndex &) const
{
    static int sectionCount = 0;
    if (sectionCount == 0) {
        // headers sections defining possible columns
        while (!headerData(sectionCount, Qt::Horizontal, Qt::DisplayRole).isNull())
            sectionCount++;
    }

    return sectionCount;
}

int WorkspaceModel::rowCount(const QModelIndex &) const
{
    return m_sortedWorkspaces.count();
}

QStringList pathsToBaseNames(const QStringList &paths)
{
    return Utils::transform(paths,
                            [](const QString &path) { return QFileInfo(path).completeBaseName(); });
}

QVariant WorkspaceModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (index.isValid()) {
        QString workspaceName = m_sortedWorkspaces.at(index.row());

        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case 0:
                result = workspaceName;
                break;
            case 1:
                result = m_manager->workspaceDateTime(workspaceName);
                break;
            } // switch (section)
            break;
        case Qt::FontRole: {
            QFont font;
            if (m_manager->isWorkspacePreset(workspaceName))
                font.setItalic(true);
            else
                font.setItalic(false);
            if (m_manager->activeWorkspace() == workspaceName)
                font.setBold(true);
            else
                font.setBold(false);
            result = font;
        } break;
        case PresetWorkspaceRole:
            result = m_manager->isWorkspacePreset(workspaceName);
            break;
        case LastWorkspaceRole:
            result = m_manager->lastWorkspace() == workspaceName;
            break;
        case ActiveWorkspaceRole:
            result = m_manager->activeWorkspace() == workspaceName;
            break;
        } // switch (role)
    }

    return result;
}

QHash<int, QByteArray> WorkspaceModel::roleNames() const
{
    static QHash<int, QByteArray> extraRoles{{Qt::DisplayRole, "workspaceName"},
                                             {PresetWorkspaceRole, "presetWorkspace"},
                                             {LastWorkspaceRole, "activeWorkspace"},
                                             {ActiveWorkspaceRole, "lastWorkspace"}};

    auto defaultRoles = QAbstractTableModel::roleNames();
    defaultRoles.insert(extraRoles);
    return defaultRoles;
}

void WorkspaceModel::sort(int column, Qt::SortOrder order)
{
    m_currentSortColumn = column;
    m_currentSortOrder = order;

    beginResetModel();
    const auto cmp = [this, column, order](const QString &s1, const QString &s2) {
        bool isLess;
        if (column == 0)
            isLess = s1 < s2;
        else
            isLess = m_manager->workspaceDateTime(s1) < m_manager->workspaceDateTime(s2);
        if (order == Qt::DescendingOrder)
            isLess = !isLess;
        return isLess;
    };
    Utils::sort(m_sortedWorkspaces, cmp);
    endResetModel();
}

void WorkspaceModel::resetWorkspaces()
{
    m_sortedWorkspaces = m_manager->workspaces();
    sort(m_currentSortColumn, m_currentSortOrder);
}

void WorkspaceModel::newWorkspace(QWidget *parent)
{
    WorkspaceNameInputDialog workspaceInputDialog(m_manager, parent);
    workspaceInputDialog.setWindowTitle(Tr::tr("New Workspace Name"));
    workspaceInputDialog.setActionText(Tr::tr("&Create"), Tr::tr("Create and &Open"));

    runWorkspaceNameInputDialog(&workspaceInputDialog, [this](const QString &newName) {
        m_manager->createWorkspace(newName);
    });
}

void WorkspaceModel::cloneWorkspace(QWidget *parent, const QString &workspace)
{
    WorkspaceNameInputDialog workspaceInputDialog(m_manager, parent);
    workspaceInputDialog.setWindowTitle(Tr::tr("New Workspace Name"));
    workspaceInputDialog.setActionText(Tr::tr("&Clone"), Tr::tr("Clone and &Open"));
    workspaceInputDialog.setValue(workspace + " (2)");

    runWorkspaceNameInputDialog(&workspaceInputDialog, [this, workspace](const QString &newName) {
        m_manager->cloneWorkspace(workspace, newName);
    });
}

void WorkspaceModel::deleteWorkspaces(const QStringList &workspaces)
{
    if (!m_manager->confirmWorkspaceDelete(workspaces))
        return;

    m_manager->deleteWorkspaces(workspaces);
    m_sortedWorkspaces = m_manager->workspaces();
    sort(m_currentSortColumn, m_currentSortOrder);
}

void WorkspaceModel::renameWorkspace(QWidget *parent, const QString &workspace)
{
    WorkspaceNameInputDialog workspaceInputDialog(m_manager, parent);
    workspaceInputDialog.setWindowTitle(Tr::tr("Rename Workspace"));
    workspaceInputDialog.setActionText(Tr::tr("&Rename"), Tr::tr("Rename and &Open"));
    workspaceInputDialog.setValue(workspace);

    runWorkspaceNameInputDialog(&workspaceInputDialog, [this, workspace](const QString &newName) {
        m_manager->renameWorkspace(workspace, newName);
    });
}

void WorkspaceModel::resetWorkspace(const QString &workspace)
{
    if (m_manager->resetWorkspacePreset(workspace) && workspace == m_manager->activeWorkspace())
        m_manager->reloadActiveWorkspace();
}

void WorkspaceModel::switchToWorkspace(const QString &workspace)
{
    m_manager->openWorkspace(workspace);
    emit workspaceSwitched();
}

void WorkspaceModel::importWorkspace(const QString &workspace)
{
    m_manager->importWorkspace(workspace);
    m_sortedWorkspaces = m_manager->workspaces();
    sort(m_currentSortColumn, m_currentSortOrder);
}

void WorkspaceModel::exportWorkspace(const QString &target, const QString &workspace)
{
    m_manager->exportWorkspace(target, workspace);
}

void WorkspaceModel::runWorkspaceNameInputDialog(WorkspaceNameInputDialog *workspaceInputDialog,
                                                 std::function<void(const QString &)> createWorkspace)
{
    if (workspaceInputDialog->exec() == QDialog::Accepted) {
        QString newWorkspace = workspaceInputDialog->value();
        if (newWorkspace.isEmpty() || m_manager->workspaces().contains(newWorkspace))
            return;

        createWorkspace(newWorkspace);
        m_sortedWorkspaces = m_manager->workspaces();
        sort(m_currentSortColumn, m_currentSortOrder);

        if (workspaceInputDialog->isSwitchToRequested())
            switchToWorkspace(newWorkspace);

        emit workspaceCreated(newWorkspace);
    }
}

} // namespace ADS
