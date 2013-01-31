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

#include "taskmodel.h"

#include "task.h"
#include "taskhub.h"

#include <utils/qtcassert.h>

#include <QFontMetrics>

namespace ProjectExplorer {
namespace Internal {

/////
// TaskModel
/////

TaskModel::TaskModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_maxSizeOfFileName(0),
    m_lastMaxSizeIndex(0),
    m_errorIcon(QLatin1String(":/projectexplorer/images/compile_error.png")),
    m_warningIcon(QLatin1String(":/projectexplorer/images/compile_warning.png")),
    m_sizeOfLineNumber(0)
{
    m_categories.insert(Core::Id(), CategoryData());
}

int TaskModel::taskCount(const Core::Id &categoryId)
{
    return m_categories.value(categoryId).count;
}

int TaskModel::errorTaskCount(const Core::Id &categoryId)
{
    return m_categories.value(categoryId).errors;
}

int TaskModel::warningTaskCount(const Core::Id &categoryId)
{
    return m_categories.value(categoryId).warnings;
}

int TaskModel::unknownTaskCount(const Core::Id &categoryId)
{
    return m_categories.value(categoryId).count
            - m_categories.value(categoryId).errors
            - m_categories.value(categoryId).warnings;
}

bool TaskModel::hasFile(const QModelIndex &index) const
{
    int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_tasks.count())
        return false;
    return !m_tasks.at(row).file.isEmpty();
}

QIcon TaskModel::taskTypeIcon(Task::TaskType t) const
{
    switch (t) {
    case Task::Warning:
        return m_warningIcon;
    case Task::Error:
        return m_errorIcon;
    case Task::Unknown:
        break;
    }
    return QIcon();
}

void TaskModel::addCategory(const Core::Id &categoryId, const QString &categoryName)
{
    QTC_ASSERT(categoryId.uniqueIdentifier(), return);
    CategoryData data;
    data.displayName = categoryName;
    m_categories.insert(categoryId, data);
}

QList<Task> TaskModel::tasks(const Core::Id &categoryId) const
{
    if (categoryId.uniqueIdentifier() == 0)
        return m_tasks;

    QList<Task> taskList;
    foreach (const Task &t, m_tasks) {
        if (t.category.uniqueIdentifier() == categoryId.uniqueIdentifier())
            taskList.append(t);
    }
    return taskList;
}

bool sortById(const Task &task, unsigned int id)
{
    return task.taskId < id;
}

void TaskModel::addTask(const Task &task)
{
    Q_ASSERT(m_categories.keys().contains(task.category));
    CategoryData &data = m_categories[task.category];
    CategoryData &global = m_categories[Core::Id()];

    QList<Task>::iterator it = qLowerBound(m_tasks.begin(), m_tasks.end(),task.taskId, sortById);
    int i = it - m_tasks.begin();
    beginInsertRows(QModelIndex(), i, i);
    m_tasks.insert(it, task);
    data.addTask(task);
    global.addTask(task);
    endInsertRows();
}

void TaskModel::removeTask(const Task &task)
{
    int index = m_tasks.indexOf(task);
    if (index >= 0) {
        const Task &t = m_tasks.at(index);

        beginRemoveRows(QModelIndex(), index, index);
        m_categories[task.category].removeTask(t);
        m_categories[Core::Id()].removeTask(t);
        m_tasks.removeAt(index);
        endRemoveRows();
    }
}

int TaskModel::rowForId(unsigned int id)
{
    QList<Task>::const_iterator it = qLowerBound(m_tasks.constBegin(), m_tasks.constEnd(), id, sortById);
    if (it == m_tasks.constEnd())
        return -1;
    return it - m_tasks.constBegin();
}

void TaskModel::updateTaskFileName(unsigned int id, const QString &fileName)
{
    int i = rowForId(id);
    QTC_ASSERT(i != -1, return);
    if (m_tasks.at(i).taskId == id) {
        m_tasks[i].file = Utils::FileName::fromString(fileName);
        emit dataChanged(index(i, 0), index(i, 0));
    }
}

void TaskModel::updateTaskLineNumber(unsigned int id, int line)
{
    int i = rowForId(id);
    QTC_ASSERT(i != -1, return);
    if (m_tasks.at(i).taskId == id) {
        m_tasks[i].movedLine = line;
        emit dataChanged(index(i, 0), index(i, 0));
    }
}

void TaskModel::clearTasks(const Core::Id &categoryId)
{
    if (categoryId.uniqueIdentifier() == 0) {
        if (m_tasks.count() == 0)
            return;
        beginRemoveRows(QModelIndex(), 0, m_tasks.count() -1);
        m_tasks.clear();
        foreach (const Core::Id &key, m_categories.keys())
            m_categories[key].clear();
        endRemoveRows();
    } else {
        int index = 0;
        int start = 0;
        CategoryData &global = m_categories[Core::Id()];
        CategoryData &cat = m_categories[categoryId];

        while (index < m_tasks.count()) {
            while (index < m_tasks.count() && m_tasks.at(index).category != categoryId) {
                ++start;
                ++index;
            }
            if (index == m_tasks.count())
                break;
            while (index < m_tasks.count() && m_tasks.at(index).category == categoryId)
                ++index;

            // Index is now on the first non category
            beginRemoveRows(QModelIndex(), start, index - 1);

            for (int i = start; i < index; ++i) {
                global.removeTask(m_tasks.at(i));
                cat.removeTask(m_tasks.at(i));
            }

            m_tasks.erase(m_tasks.begin() + start, m_tasks.begin() + index);

            endRemoveRows();
            index = start;
        }
    }
    m_maxSizeOfFileName = 0;
    m_lastMaxSizeIndex = 0;
}

QModelIndex TaskModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex TaskModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_tasks.count();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
        return parent.isValid() ? 0 : 1;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tasks.count() || index.column() != 0)
        return QVariant();

    if (role == TaskModel::File)
        return m_tasks.at(index.row()).file.toString();
    else if (role == TaskModel::Line)
        return m_tasks.at(index.row()).line;
    else if (role == TaskModel::MovedLine)
        return m_tasks.at(index.row()).movedLine;
    else if (role == TaskModel::Description)
        return m_tasks.at(index.row()).description;
    else if (role == TaskModel::FileNotFound)
        return m_fileNotFound.value(m_tasks.at(index.row()).file.toString());
    else if (role == TaskModel::Type)
        return (int)m_tasks.at(index.row()).type;
    else if (role == TaskModel::Category)
        return m_tasks.at(index.row()).category.uniqueIdentifier();
    else if (role == TaskModel::Icon)
        return taskTypeIcon(m_tasks.at(index.row()).type);
    else if (role == TaskModel::Task_t)
        return QVariant::fromValue(task(index));
    return QVariant();
}

Task TaskModel::task(const QModelIndex &index) const
{
    if (!index.isValid())
        return Task();
    return m_tasks.at(index.row());
}

QList<Core::Id> TaskModel::categoryIds() const
{
    QList<Core::Id> categories = m_categories.keys();
    categories.removeAll(Core::Id()); // remove global category we added for bookkeeping
    return categories;
}

QString TaskModel::categoryDisplayName(const Core::Id &categoryId) const
{
    return m_categories.value(categoryId).displayName;
}

int TaskModel::sizeOfFile(const QFont &font)
{
    int count = m_tasks.count();
    if (count == 0)
        return 0;

    if (m_maxSizeOfFileName > 0 && font == m_fileMeasurementFont && m_lastMaxSizeIndex == count - 1)
        return m_maxSizeOfFileName;

    QFontMetrics fm(font);
    m_fileMeasurementFont = font;

    for (int i = m_lastMaxSizeIndex; i < count; ++i) {
        QString filename = m_tasks.at(i).file.toString();
        const int pos = filename.lastIndexOf(QLatin1Char('/'));
        if (pos != -1)
            filename = filename.mid(pos +1);

        m_maxSizeOfFileName = qMax(m_maxSizeOfFileName, fm.width(filename));
    }
    m_lastMaxSizeIndex = count - 1;
    return m_maxSizeOfFileName;
}

int TaskModel::sizeOfLineNumber(const QFont &font)
{
    if (m_sizeOfLineNumber == 0 || font != m_lineMeasurementFont) {
        QFontMetrics fm(font);
        m_lineMeasurementFont = font;
        m_sizeOfLineNumber = fm.width(QLatin1String("88888"));
    }
    return m_sizeOfLineNumber;
}

void TaskModel::setFileNotFound(const QModelIndex &idx, bool b)
{
    if (idx.isValid() && idx.row() < m_tasks.count()) {
        m_fileNotFound.insert(m_tasks[idx.row()].file.toUserOutput(), b);
        emit dataChanged(idx, idx);
    }
}

/////
// TaskFilterModel
/////

TaskFilterModel::TaskFilterModel(TaskModel *sourceModel, QObject *parent) : QAbstractItemModel(parent),
    m_sourceModel(sourceModel)
{
    Q_ASSERT(m_sourceModel);
    connect(m_sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(handleNewRows(QModelIndex,int,int)));
    connect(m_sourceModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(handleRowsAboutToBeRemoved(QModelIndex,int,int)));
    connect(m_sourceModel, SIGNAL(modelReset()),
            this, SLOT(handleReset()));
    connect(m_sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(handleDataChanged(QModelIndex,QModelIndex)));

    m_includeUnknowns = m_includeWarnings = m_includeErrors = true;
}

QModelIndex TaskFilterModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex TaskFilterModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int TaskFilterModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    updateMapping();
    return m_mapping.count();
}

int TaskFilterModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_sourceModel->columnCount(parent);
}

QVariant TaskFilterModel::data(const QModelIndex &index, int role) const
{
    return m_sourceModel->data(mapToSource(index), role);
}

static QPair<int, int> findFilteredRange(int first, int last, const QList<int> &list)
{
    QList<int>::const_iterator filteredFirst = qLowerBound(list, first);
    QList<int>::const_iterator filteredLast = qUpperBound(filteredFirst, list.constEnd(), last);
    return qMakePair(filteredFirst - list.constBegin(), filteredLast - list.constBegin() - 1);
}

void TaskFilterModel::handleNewRows(const QModelIndex &index, int first, int last)
{
    if (index.isValid())
        return;

    QList<int> newMapping;
    for (int i = first; i <= last; ++i) {
        const Task &task = m_sourceModel->task(m_sourceModel->index(i, 0));
        if (filterAcceptsTask(task))
            newMapping.append(i);
    }

    const int newItems = newMapping.count();
    if (!newItems)
        return;

    int filteredFirst = -1;
    if (last == m_sourceModel->rowCount() - 1)
        filteredFirst = m_mapping.count();
    else
        filteredFirst = qLowerBound(m_mapping, first) - m_mapping.constBegin();

    const int filteredLast = filteredFirst + newItems - 1;
    beginInsertRows(QModelIndex(), filteredFirst, filteredLast);
    if (filteredFirst == m_mapping.count()) {
        m_mapping.append(newMapping);
    } else {
        QList<int> rest = m_mapping.mid(filteredFirst);

        m_mapping.reserve(m_mapping.count() + newItems);
        m_mapping.erase(m_mapping.begin() + filteredFirst, m_mapping.end());
        m_mapping.append(newMapping);
        foreach (int pos, rest)
            m_mapping.append(pos + newItems);
    }
    endInsertRows();
}

void TaskFilterModel::handleRowsAboutToBeRemoved(const QModelIndex &index, int first, int last)
{
    if (index.isValid())
        return;

    const QPair<int, int> range = findFilteredRange(first, last, m_mapping);
    if (range.first > range.second)
        return;

    beginRemoveRows(QModelIndex(), range.first, range.second);
    m_mapping.erase(m_mapping.begin() + range.first, m_mapping.begin() + range.second + 1);
    for (int i = range.first; i < m_mapping.count(); ++i)
        m_mapping[i] = m_mapping.at(i) - (last - first) - 1;
    endRemoveRows();
}

void TaskFilterModel::handleDataChanged(QModelIndex top, QModelIndex bottom)
{
    const QPair<int, int> range = findFilteredRange(top.row(), bottom.row(), m_mapping);
    if (range.first > range.second)
        return;

    emit dataChanged(index(range.first, top.column()), index(range.second, bottom.column()));
}

void TaskFilterModel::handleReset()
{
    invalidateFilter();
}

QModelIndex TaskFilterModel::mapFromSource(const QModelIndex &idx) const
{
    updateMapping();
    QList<int>::const_iterator it = qBinaryFind(m_mapping.constBegin(), m_mapping.constEnd(), idx.row());
    if (it == m_mapping.constEnd())
        return QModelIndex();
    return index(it - m_mapping.constBegin(), 0);
}

QModelIndex TaskFilterModel::mapToSource(const QModelIndex &index) const
{
    updateMapping();
    int row = index.row();
    if (row >= m_mapping.count())
        return QModelIndex();
    return m_sourceModel->index(m_mapping.at(row), index.column(), index.parent());
}

void TaskFilterModel::invalidateFilter()
{
    beginResetModel();
    m_mappingUpToDate = false;
    endResetModel();
}

void TaskFilterModel::updateMapping() const
{
    if (m_mappingUpToDate)
        return;

    m_mapping.clear();
    for (int i = 0; i < m_sourceModel->rowCount(); ++i) {
        QModelIndex index = m_sourceModel->index(i, 0);
        const Task &task = m_sourceModel->task(index);
        if (filterAcceptsTask(task))
            m_mapping.append(i);
    }

    m_mappingUpToDate = true;
}

bool TaskFilterModel::filterAcceptsTask(const Task &task) const
{
    bool accept = true;
    switch (task.type) {
    case Task::Unknown:
        accept = m_includeUnknowns;
        break;
    case Task::Warning:
        accept = m_includeWarnings;
        break;
    case Task::Error:
        accept = m_includeErrors;
        break;
    }

    if (m_categoryIds.contains(task.category))
        accept = false;

    return accept;
}

} // namespace Internal
} // namespace ProjectExplorer
