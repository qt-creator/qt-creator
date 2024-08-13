// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "messagemodel.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>

MessageModel::MessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
    setupTaskHub();
}

int MessageModel::errorCount() const
{
    int count = 0;
    for (const auto &task : m_tasks) {
        if (task.type == ProjectExplorer::Task::TaskType::Error)
            count++;
    }
    return count;
}

int MessageModel::warningCount() const
{
    int count = 0;
    for (const auto &task : m_tasks) {
        if (task.type == ProjectExplorer::Task::TaskType::Warning)
            count++;
    }
    return count;
}

void MessageModel::resetModel()
{
    auto *hub = &ProjectExplorer::taskHub();
    hub->clearTasks();

    beginResetModel();
    m_tasks.clear();
    endResetModel();
    emit modelChanged();
}

void MessageModel::jumpToCode(const QVariant &index)
{
    bool ok = false;
    if (int idx = index.toInt(&ok); ok) {
        if (idx >= 0 && idx < static_cast<int>(m_tasks.size())) {
            // TODO:
            // - Check why this does not jump to line/column
            // - Only call this when sure that the task, file row etc are valid.
            ProjectExplorer::Task task = m_tasks.at(idx);
            const int column = task.column ? task.column - 1 : 0;

            Utils::Link link(task.file, task.line, column);
            Core::EditorManager::openEditorAt(link,
                                              {},
                                              Core::EditorManager::SwitchSplitIfAlreadyVisible);
        }
    }
}

int MessageModel::rowCount(const QModelIndex &) const
{
    return m_tasks.size();
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    QHash<int, QByteArray> out;
    out[MessageRole] = "message";
    out[FileNameRole] = "location";
    out[TypeRole] = "type";
    return out;
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount()) {
        int row = index.row();
        if (role == MessageRole) {
            return m_tasks.at(row).description();
        } else if (role == FileNameRole) {
            Utils::FilePath path = m_tasks.at(row).file;
            return path.fileName();
        } else if (role == TypeRole) {
            ProjectExplorer::Task::TaskType type = m_tasks.at(row).type;
            if (type == ProjectExplorer::Task::TaskType::Error)
                return "Error";
            else if (type == ProjectExplorer::Task::TaskType::Warning)
                return "Warning";
            else
                return "Unknown";
        } else {
            qWarning() << Q_FUNC_INFO << "invalid role";
        }
    } else {
        qWarning() << Q_FUNC_INFO << "invalid index";
    }
    return QVariant();
}

void MessageModel::setupTaskHub()
{
    auto *hub = &ProjectExplorer::taskHub();
    connect(hub, &ProjectExplorer::TaskHub::categoryAdded, this, &MessageModel::addCategory);
    connect(hub, &ProjectExplorer::TaskHub::taskAdded, this, &MessageModel::addTask);
    connect(hub, &ProjectExplorer::TaskHub::taskRemoved, this, &MessageModel::removeTask);
}

void MessageModel::addCategory(const ProjectExplorer::TaskCategory &category)
{
    m_categories[category.id.uniqueIdentifier()] = category;
}

void MessageModel::addTask(const ProjectExplorer::Task &task)
{
    int at = m_tasks.size();
    beginInsertRows(QModelIndex(), at, at);
    m_tasks.push_back(task);
    endInsertRows();
    emit modelChanged();
}

void MessageModel::removeTask(const ProjectExplorer::Task &task)
{
    for (int i = 0; i < static_cast<int>(m_tasks.size()); i++) {
        if (m_tasks.at(i) == task) {
            beginRemoveRows(QModelIndex(), i, i);
            m_tasks.erase(m_tasks.begin() + i);
            endRemoveRows();
            emit modelChanged();
            return;
        }
    }
}
