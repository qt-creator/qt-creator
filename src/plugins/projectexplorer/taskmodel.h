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

#pragma once

#include <QAbstractItemModel>

#include <QIcon>

#include "task.h"

namespace ProjectExplorer {
namespace Internal {

class TaskModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    // Model stuff
    explicit TaskModel(QObject *parent);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Task task(const QModelIndex &index) const;

    QList<Core::Id> categoryIds() const;
    QString categoryDisplayName(Core::Id categoryId) const;
    void addCategory(Core::Id categoryId, const QString &categoryName);

    QList<Task> tasks(Core::Id categoryId = Core::Id()) const;
    void addTask(const Task &task);
    void removeTask(const Task &task);
    void clearTasks(Core::Id categoryId = Core::Id());
    void updateTaskFileName(unsigned int id, const QString &fileName);
    void updateTaskLineNumber(unsigned int id, int line);

    int sizeOfFile(const QFont &font);
    int sizeOfLineNumber(const QFont &font);
    void setFileNotFound(const QModelIndex &index, bool b);

    enum Roles { File = Qt::UserRole, Line, MovedLine, Description, FileNotFound, Type, Category, Icon, Task_t };

    int taskCount(Core::Id categoryId);
    int errorTaskCount(Core::Id categoryId);
    int warningTaskCount(Core::Id categoryId);
    int unknownTaskCount(Core::Id categoryId);

    bool hasFile(const QModelIndex &index) const;

    int rowForId(unsigned int id);
private:

    class CategoryData
    {
    public:
        void addTask(const Task &task)
        {
            ++count;
            if (task.type == Task::Warning)
                ++warnings;
            else if (task.type == Task::Error)
                ++errors;
        }

        void removeTask(const Task &task)
        {
            --count;
            if (task.type == Task::Warning)
                --warnings;
            else if (task.type == Task::Error)
                --errors;
        }

        void clear() {
            count = 0;
            warnings = 0;
            errors = 0;
        }

        QString displayName;
        int count = 0;
        int warnings = 0;
        int errors = 0;
    };

    QHash<Core::Id,CategoryData> m_categories; // category id to data
    QList<Task> m_tasks;   // all tasks (in order of id)

    QHash<QString,bool> m_fileNotFound;
    QFont m_fileMeasurementFont;
    QFont m_lineMeasurementFont;
    int m_maxSizeOfFileName = 0;
    int m_lastMaxSizeIndex = 0;
    int m_sizeOfLineNumber = 0;
};

class TaskFilterModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TaskFilterModel(TaskModel *sourceModel, QObject *parent = nullptr);

    TaskModel *taskModel() { return m_sourceModel; }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool filterIncludesUnknowns() const { return m_includeUnknowns; }
    void setFilterIncludesUnknowns(bool b) { m_includeUnknowns = b; invalidateFilter(); }

    bool filterIncludesWarnings() const { return m_includeWarnings; }
    void setFilterIncludesWarnings(bool b) { m_includeWarnings = b; invalidateFilter(); }

    bool filterIncludesErrors() const { return m_includeErrors; }
    void setFilterIncludesErrors(bool b) { m_includeErrors = b; invalidateFilter(); }

    QList<Core::Id> filteredCategories() const { return m_categoryIds; }
    void setFilteredCategories(const QList<Core::Id> &categoryIds) { m_categoryIds = categoryIds; invalidateFilter(); }

    Task task(const QModelIndex &index) const
    { return m_sourceModel->task(mapToSource(index)); }

    bool hasFile(const QModelIndex &index) const
    { return m_sourceModel->hasFile(mapToSource(index)); }

    QModelIndex mapFromSource(const QModelIndex &idx) const;

private:
    void handleNewRows(const QModelIndex &index, int first, int last);
    void handleRowsAboutToBeRemoved(const QModelIndex &index, int first, int last);
    void handleDataChanged(const QModelIndex &top, const QModelIndex &bottom);
    void handleReset();

    QModelIndex mapToSource(const QModelIndex &index) const;
    void invalidateFilter();
    void updateMapping() const;
    bool filterAcceptsTask(const Task &task) const;

    bool m_includeUnknowns;
    bool m_includeWarnings;
    bool m_includeErrors;
    QList<Core::Id> m_categoryIds;

    mutable QList<int> m_mapping;

    TaskModel *m_sourceModel;
};

} // namespace Internal
} // namespace ProjectExplorer
