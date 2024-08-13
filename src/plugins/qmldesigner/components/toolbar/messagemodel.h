// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>

class MessageModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int errorCount READ errorCount NOTIFY modelChanged)
    Q_PROPERTY(int warningCount READ warningCount NOTIFY modelChanged)

    enum {
        TypeRole = Qt::DecorationRole,
        MessageRole = Qt::DisplayRole,
        FileNameRole = Qt::UserRole,
    };

signals:
    void modelChanged();

public:
    MessageModel(QObject *parent = nullptr);

    int errorCount() const;
    int warningCount() const;
    Q_INVOKABLE void resetModel();
    Q_INVOKABLE void jumpToCode(const QVariant &index);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = MessageRole) const override;

private:
    void setupTaskHub();
    void addCategory(const ProjectExplorer::TaskCategory &category);
    void addTask(const ProjectExplorer::Task &task);
    void removeTask(const ProjectExplorer::Task &task);

    std::vector<ProjectExplorer::Task> m_tasks = {};
    std::unordered_map<quintptr, ProjectExplorer::TaskCategory> m_categories = {};
};
