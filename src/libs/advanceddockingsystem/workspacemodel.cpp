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
{
    connect(m_manager, &DockManager::workspaceLoaded, this, &WorkspaceModel::resetWorkspaces);
}

int WorkspaceModel::indexOfWorkspace(const QString &fileName)
{
    return m_manager->workspaceIndex(fileName);
}

QString WorkspaceModel::workspaceAt(int row) const
{
    if (row >= rowCount() || row < 0)
        return {};

    return m_manager->workspaces()[row].fileName();
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
                result = Tr::tr("File Name");
                break;
            case 2:
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
        while (!headerData(sectionCount, Qt::Horizontal, Qt::DisplayRole).isNull())
            sectionCount++;
    }

    return sectionCount;
}

int WorkspaceModel::rowCount(const QModelIndex &) const
{
    return m_manager->workspaces().count();
}

QVariant WorkspaceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    Workspace *workspace = m_manager->workspace(index.row());
    if (!workspace)
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return workspace->name();
        case 1:
            return workspace->fileName();
        case 2:
            return workspace->lastModified();
        default:
            qWarning("data: invalid display value column %d", index.column());
            break;
        }
        break;
    case Qt::FontRole: {
        QFont font;
        font.setItalic(workspace->isPreset());
        font.setBold(*m_manager->activeWorkspace() == *workspace);
        return font;
    }
    case WorkspacePreset:
        return workspace->isPreset();
    case WorkspaceStartup:
        return m_manager->startupWorkspace() == workspace->fileName();
    case WorkspaceActive:
        return *m_manager->activeWorkspace() == *workspace;
    }
    return QVariant();
}

QHash<int, QByteArray> WorkspaceModel::roleNames() const
{
    static QHash<int, QByteArray> extraRoles{{Qt::DisplayRole, "workspaceName"},
                                             {WorkspacePreset, "presetWorkspace"},
                                             {WorkspaceStartup, "lastWorkspace"},
                                             {WorkspaceActive, "activeWorkspace"}};

    auto defaultRoles = QAbstractTableModel::roleNames();
    defaultRoles.insert(extraRoles);
    return defaultRoles;
}

Qt::DropActions WorkspaceModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

Qt::ItemFlags WorkspaceModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

    if (index.isValid())
        return Qt::ItemIsDragEnabled | defaultFlags;

    return Qt::ItemIsDropEnabled | defaultFlags;
}

void WorkspaceModel::resetWorkspaces()
{
    beginResetModel();
    endResetModel();
}

} // namespace ADS
