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

#include "taskmodel.h"

#include "fileinsessionfinder.h"
#include "task.h"
#include "taskhub.h"

#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QFontMetrics>

#include <algorithm>

namespace ProjectExplorer {
namespace Internal {

/////
// TaskModel
/////

TaskModel::TaskModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_categories.insert(Utils::Id(), CategoryData());
}

int TaskModel::taskCount(Utils::Id categoryId)
{
    return m_categories.value(categoryId).count;
}

int TaskModel::errorTaskCount(Utils::Id categoryId)
{
    return m_categories.value(categoryId).errors;
}

int TaskModel::warningTaskCount(Utils::Id categoryId)
{
    return m_categories.value(categoryId).warnings;
}

int TaskModel::unknownTaskCount(Utils::Id categoryId)
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

void TaskModel::addCategory(Utils::Id categoryId, const QString &categoryName)
{
    QTC_ASSERT(categoryId.isValid(), return);
    CategoryData data;
    data.displayName = categoryName;
    m_categories.insert(categoryId, data);
}

Tasks TaskModel::tasks(Utils::Id categoryId) const
{
    if (!categoryId.isValid())
        return m_tasks;

    Tasks taskList;
    foreach (const Task &t, m_tasks) {
        if (t.category == categoryId)
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
    CategoryData &global = m_categories[Utils::Id()];

    auto it = std::lower_bound(m_tasks.begin(), m_tasks.end(),task.taskId, sortById);
    int i = it - m_tasks.begin();
    beginInsertRows(QModelIndex(), i, i);
    m_tasks.insert(it, task);
    data.addTask(task);
    global.addTask(task);
    endInsertRows();
}

void TaskModel::removeTask(unsigned int id)
{
    for (int index = 0; index < m_tasks.length(); ++index) {
        if (m_tasks.at(index).taskId != id)
            continue;
        const Task &t = m_tasks.at(index);
        beginRemoveRows(QModelIndex(), index, index);
        m_categories[t.category].removeTask(t);
        m_categories[Utils::Id()].removeTask(t);
        m_tasks.removeAt(index);
        endRemoveRows();
        break;
    }
}

int TaskModel::rowForId(unsigned int id)
{
    auto it = std::lower_bound(m_tasks.constBegin(), m_tasks.constEnd(), id, sortById);
    if (it == m_tasks.constEnd())
        return -1;
    return it - m_tasks.constBegin();
}

void TaskModel::updateTaskFileName(unsigned int id, const QString &fileName)
{
    int i = rowForId(id);
    QTC_ASSERT(i != -1, return);
    if (m_tasks.at(i).taskId == id) {
        m_tasks[i].file = Utils::FilePath::fromString(fileName);
        const QModelIndex itemIndex = index(i, 0);
        emit dataChanged(itemIndex, itemIndex);
    }
}

void TaskModel::updateTaskLineNumber(unsigned int id, int line)
{
    int i = rowForId(id);
    QTC_ASSERT(i != -1, return);
    if (m_tasks.at(i).taskId == id) {
        m_tasks[i].movedLine = line;
        const QModelIndex itemIndex = index(i, 0);
        emit dataChanged(itemIndex, itemIndex);
    }
}

void TaskModel::clearTasks(Utils::Id categoryId)
{
    using IdCategoryConstIt = QHash<Utils::Id,CategoryData>::ConstIterator;

    if (!categoryId.isValid()) {
        if (m_tasks.isEmpty())
            return;
        beginRemoveRows(QModelIndex(), 0, m_tasks.count() -1);
        m_tasks.clear();
        const IdCategoryConstIt cend = m_categories.constEnd();
        for (IdCategoryConstIt it = m_categories.constBegin(); it != cend; ++it)
            m_categories[it.key()].clear();
        endRemoveRows();
    } else {
        int index = 0;
        int start = 0;
        CategoryData &global = m_categories[Utils::Id()];
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
    int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_tasks.count() || index.column() != 0)
        return QVariant();

    if (role == TaskModel::File)
        return m_tasks.at(index.row()).file.toString();
    else if (role == TaskModel::Line)
        return m_tasks.at(index.row()).line;
    else if (role == TaskModel::MovedLine)
        return m_tasks.at(index.row()).movedLine;
    else if (role == TaskModel::Description)
        return m_tasks.at(index.row()).description();
    else if (role == TaskModel::FileNotFound)
        return m_fileNotFound.value(m_tasks.at(index.row()).file.toString());
    else if (role == TaskModel::Type)
        return (int)m_tasks.at(index.row()).type;
    else if (role == TaskModel::Category)
        return m_tasks.at(index.row()).category.uniqueIdentifier();
    else if (role == TaskModel::Icon)
        return m_tasks.at(index.row()).icon;
    else if (role == TaskModel::Task_t)
        return QVariant::fromValue(task(index));
    return QVariant();
}

Task TaskModel::task(const QModelIndex &index) const
{
    int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_tasks.count())
        return Task();
    return m_tasks.at(row);
}

QList<Utils::Id> TaskModel::categoryIds() const
{
    QList<Utils::Id> categories = m_categories.keys();
    categories.removeAll(Utils::Id()); // remove global category we added for bookkeeping
    return categories;
}

QString TaskModel::categoryDisplayName(Utils::Id categoryId) const
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

        m_maxSizeOfFileName = qMax(m_maxSizeOfFileName, fm.horizontalAdvance(filename));
    }
    m_lastMaxSizeIndex = count - 1;
    return m_maxSizeOfFileName;
}

int TaskModel::sizeOfLineNumber(const QFont &font)
{
    if (m_sizeOfLineNumber == 0 || font != m_lineMeasurementFont) {
        QFontMetrics fm(font);
        m_lineMeasurementFont = font;
        m_sizeOfLineNumber = fm.horizontalAdvance(QLatin1String("88888"));
    }
    return m_sizeOfLineNumber;
}

void TaskModel::setFileNotFound(const QModelIndex &idx, bool b)
{
    int row = idx.row();
    if (!idx.isValid() || row < 0 || row >= m_tasks.count())
        return;
    m_fileNotFound.insert(m_tasks[row].file.toUserOutput(), b);
    emit dataChanged(idx, idx);
}

/////
// TaskFilterModel
/////

TaskFilterModel::TaskFilterModel(TaskModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    QTC_ASSERT(sourceModel, return);
    setSourceModel(sourceModel);
    m_includeUnknowns = m_includeWarnings = m_includeErrors = true;
}

void TaskFilterModel::setFilterIncludesWarnings(bool b)
{
    m_includeWarnings = b;
    m_includeUnknowns = b; // "Unknowns" are often associated with warnings
    invalidateFilter();
}

int TaskFilterModel::issuesCount(int startRow, int endRow) const
{
    int count = 0;
    for (int r = startRow; r <= endRow; ++r) {
        if (task(index(r, 0)).type != Task::Unknown)
            ++count;
    }
    return count;
}

void TaskFilterModel::updateFilterProperties(
        const QString &filterText,
        Qt::CaseSensitivity caseSensitivity,
        bool isRegexp,
        bool isInverted)
{
    if (filterText == m_filterText && m_filterCaseSensitivity == caseSensitivity
            && m_filterStringIsRegexp == isRegexp && m_filterIsInverted == isInverted) {
        return;
    }
    m_filterText = filterText;
    m_filterCaseSensitivity = caseSensitivity;
    m_filterStringIsRegexp = isRegexp;
    m_filterIsInverted = isInverted;
    if (m_filterStringIsRegexp) {
        m_filterRegexp.setPattern(m_filterText);
        m_filterRegexp.setPatternOptions(m_filterCaseSensitivity == Qt::CaseInsensitive
                                         ? QRegularExpression::CaseInsensitiveOption
                                         : QRegularExpression::NoPatternOption);
    }
    invalidateFilter();
}

bool TaskFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)
    return filterAcceptsTask(taskModel()->tasks().at(source_row));
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

    if (accept && m_categoryIds.contains(task.category))
        accept = false;

    if (accept && !m_filterText.isEmpty()) {
        const auto accepts = [this](const QString &s) {
            return m_filterStringIsRegexp ? m_filterRegexp.isValid() && s.contains(m_filterRegexp)
                                          : s.contains(m_filterText, m_filterCaseSensitivity);
        };
        if ((accepts(task.file.toString()) || accepts(task.description())) == m_filterIsInverted)
            accept = false;
    }

    return accept;
}

} // namespace Internal
} // namespace ProjectExplorer
