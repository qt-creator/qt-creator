// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSortFilterProxyModel>

#include <QIcon>
#include <QRegularExpression>

#include "task.h"
#include "taskhub.h"

namespace ProjectExplorer {
class TaskCategory;

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
    Tasks tasks(const QModelIndexList &indexes) const;

    QList<TaskCategory> categories() const;
    void addCategory(const TaskCategory &category);

    Tasks tasks(Utils::Id categoryId = Utils::Id()) const;
    void addTask(const Task &t);
    void removeTask(unsigned int id);
    void clearTasks(Utils::Id categoryId = Utils::Id());
    void updateTaskFileName(const Task &task, const QString &fileName);
    void updateTaskLineNumber(const Task &task, int line);

    int sizeOfFile(const QFont &font);
    int sizeOfLineNumber(const QFont &font);
    void setFileNotFound(const QModelIndex &index, bool b);

    enum Roles { Description = Qt::UserRole, Type};

    int taskCount(Utils::Id categoryId);
    int errorTaskCount(Utils::Id categoryId);
    int warningTaskCount(Utils::Id categoryId);
    int unknownTaskCount(Utils::Id categoryId);

    bool hasFile(const QModelIndex &index) const;

    int rowForTask(const Task &task);
private:
    bool compareTasks(const Task &t1, const Task &t2);

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

        TaskCategory category;
        int count = 0;
        int warnings = 0;
        int errors = 0;
    };

    QHash<Utils::Id,CategoryData> m_categories; // category id to data
    Tasks m_tasks;   // all tasks (in order of id)

    QHash<QString,bool> m_fileNotFound;
    QFont m_fileMeasurementFont;
    QFont m_lineMeasurementFont;
    int m_maxSizeOfFileName = 0;
    int m_lastMaxSizeIndex = 0;
    int m_sizeOfLineNumber = 0;
};

class TaskFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    TaskFilterModel(TaskModel *sourceModel, QObject *parent = nullptr);

    TaskModel *taskModel() const { return static_cast<TaskModel *>(sourceModel()); }

    bool filterIncludesWarnings() const { return m_includeWarnings; }
    void setFilterIncludesWarnings(bool b);

    bool filterIncludesErrors() const { return m_includeErrors; }
    void setFilterIncludesErrors(bool b) { m_includeErrors = b; invalidateFilter(); }

    QSet<Utils::Id> filteredCategories() const { return m_categoryIds; }
    void setFilteredCategories(const QSet<Utils::Id> &categoryIds)
    {
        m_categoryIds = categoryIds;
        invalidateFilter();
    }

    Task task(const QModelIndex &index) const { return taskModel()->task(mapToSource(index)); }
    Tasks tasks(const QModelIndexList &indexes) const;
    int issuesCount(int startRow, int endRow) const;

    bool hasFile(const QModelIndex &index) const
    { return taskModel()->hasFile(mapToSource(index)); }

    void updateFilterProperties(
            const QString &filterText,
            Qt::CaseSensitivity caseSensitivity,
            bool isRegex,
            bool isInverted);

private:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool filterAcceptsTask(const Task &task) const;

    bool m_beginRemoveRowsSent = false;
    bool m_includeUnknowns;
    bool m_includeWarnings;
    bool m_includeErrors;
    bool m_filterStringIsRegexp = false;
    bool m_filterIsInverted = false;
    Qt::CaseSensitivity m_filterCaseSensitivity = Qt::CaseInsensitive;
    QSet<Utils::Id> m_categoryIds;
    QString m_filterText;
    QRegularExpression m_filterRegexp;
};

} // namespace Internal
} // namespace ProjectExplorer
