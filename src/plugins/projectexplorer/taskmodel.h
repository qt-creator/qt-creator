/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TASKMODEL_H
#define TASKMODEL_H

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
    TaskModel(QObject *parent);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Task task(const QModelIndex &index) const;

    QList<Core::Id> categoryIds() const;
    QString categoryDisplayName(const Core::Id &categoryId) const;
    void addCategory(const Core::Id &categoryId, const QString &categoryName);

    QList<Task> tasks(const Core::Id &categoryId = Core::Id()) const;
    void addTask(const Task &task);
    void removeTask(const Task &task);
    void clearTasks(const Core::Id &categoryId = Core::Id());
    void updateTaskFileName(unsigned int id, const QString &fileName);
    void updateTaskLineNumber(unsigned int id, int line);

    int sizeOfFile(const QFont &font);
    int sizeOfLineNumber(const QFont &font);
    void setFileNotFound(const QModelIndex &index, bool b);

    enum Roles { File = Qt::UserRole, Line, MovedLine, Description, FileNotFound, Type, Category, Icon, Task_t };

    QIcon taskTypeIcon(Task::TaskType t) const;

    int taskCount(const Core::Id &categoryId);
    int errorTaskCount(const Core::Id &categoryId);
    int warningTaskCount(const Core::Id &categoryId);
    int unknownTaskCount(const Core::Id &categoryId);

    bool hasFile(const QModelIndex &index) const;

    int rowForId(unsigned int id);
private:

    class CategoryData
    {
    public:
        CategoryData() : count(0), warnings(0), errors(0) { }

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
        int count;
        int warnings;
        int errors;
    };

    QHash<Core::Id,CategoryData> m_categories; // category id to data
    QList<Task> m_tasks;   // all tasks (in order of id)

    QHash<QString,bool> m_fileNotFound;
    int m_maxSizeOfFileName;
    int m_lastMaxSizeIndex;
    QFont m_fileMeasurementFont;
    const QIcon m_errorIcon;
    const QIcon m_warningIcon;
    int m_sizeOfLineNumber;
    QFont m_lineMeasurementFont;
};

class TaskFilterModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TaskFilterModel(TaskModel *sourceModel, QObject *parent = 0);

    TaskModel *taskModel() { return m_sourceModel; }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

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
private slots:
    void handleNewRows(const QModelIndex &index, int first, int last);
    void handleRowsAboutToBeRemoved(const QModelIndex &index, int first, int last);
    void handleDataChanged(QModelIndex,QModelIndex bottom);
    void handleReset();

private:
    QModelIndex mapToSource(const QModelIndex &index) const;
    void invalidateFilter();
    void updateMapping() const;
    bool filterAcceptsTask(const Task &task) const;

    bool m_includeUnknowns;
    bool m_includeWarnings;
    bool m_includeErrors;
    QList<Core::Id> m_categoryIds;

    mutable QList<int> m_mapping;
    mutable bool m_mappingUpToDate;

    TaskModel *m_sourceModel;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TASKMODEL_H
