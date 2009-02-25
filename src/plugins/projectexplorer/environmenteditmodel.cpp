/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "environmenteditmodel.h"

using namespace ProjectExplorer;

EnvironmentModel::EnvironmentModel()
    : m_mergedEnvironments(false)
{}

EnvironmentModel::~EnvironmentModel()
{}

QString EnvironmentModel::indexToVariable(const QModelIndex &index) const
{
    if (m_mergedEnvironments)
        return m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row());
    else
        return m_items.at(index.row()).name;
}

void EnvironmentModel::updateResultEnvironment()
{
    m_resultEnvironment = m_baseEnvironment;
    m_resultEnvironment.modify(m_items);
    foreach (const EnvironmentItem &item, m_items) {
        if (item.unset) {
            m_resultEnvironment.set(item.name, "<UNSET>");
        }
    }
}

void EnvironmentModel::setBaseEnvironment(const ProjectExplorer::Environment &env)
{
    m_baseEnvironment = env;
    updateResultEnvironment();
    reset();
}

void EnvironmentModel::setMergedEnvironments(bool b)
{
    if (m_mergedEnvironments == b)
        return;
    m_mergedEnvironments = b;
    if (b)
        updateResultEnvironment();
    reset();
}

bool EnvironmentModel::mergedEnvironments()
{
    return m_mergedEnvironments;
}

int EnvironmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_mergedEnvironments ? m_resultEnvironment.size() : m_items.count();
}
int EnvironmentModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

bool EnvironmentModel::changes(const QString &name) const
{
    foreach (const EnvironmentItem& item, m_items)
        if (item.name == name)
            return true;
    return false;
}

QVariant EnvironmentModel::data(const QModelIndex &index, int role) const
{
    if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.isValid()) {
        if ((m_mergedEnvironments && index.row() >= m_resultEnvironment.size()) ||
           (!m_mergedEnvironments && index.row() >= m_items.count())) {
            return QVariant();
        }

        if (index.column() == 0) {
            if (m_mergedEnvironments) {
                return m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row());
            } else {
                return m_items.at(index.row()).name;
            }
        } else if (index.column() == 1) {
            if (m_mergedEnvironments) {
                if (role == Qt::EditRole) {
                    int pos = findInChanges(indexToVariable(index));
                    if (pos != -1)
                        return m_items.at(pos).value;
                }
                return m_resultEnvironment.value(m_resultEnvironment.constBegin() + index.row());
            } else {
                if (m_items.at(index.row()).unset)
                    return "<UNSET>";
                else
                    return m_items.at(index.row()).value;
            }
        }
    }
    if (role == Qt::FontRole) {
        if (m_mergedEnvironments) {
            // check wheter this environment variable exists in m_items
            if (changes(m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row()))) {
                QFont f;
                f.setBold(true);
                return QVariant(f);
            }
        }
        return QFont();
    }
    return QVariant();
}


Qt::ItemFlags EnvironmentModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

bool EnvironmentModel::hasChildren(const QModelIndex &index) const
{
    if (!index.isValid())
        return true;
    else
        return false;
}

QVariant EnvironmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Variable") : tr("Value");
}

QModelIndex EnvironmentModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, 0);
    return QModelIndex();
}

QModelIndex EnvironmentModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

/// *****************
/// Utility functions
/// *****************
int EnvironmentModel::findInChanges(const QString &name) const
{
    for (int i=0; i<m_items.size(); ++i)
        if (m_items.at(i).name == name)
            return i;
    return -1;
}

int EnvironmentModel::findInChangesInsertPosition(const QString &name) const
{
    for (int i=0; i<m_items.size(); ++i)
        if (m_items.at(i).name > name)
            return i;
    return m_items.size();
}

int EnvironmentModel::findInResult(const QString &name) const
{
    Environment::const_iterator it;
    int i = 0;
    for (it = m_resultEnvironment.constBegin(); it != m_resultEnvironment.constEnd(); ++it, ++i)
        if (m_resultEnvironment.key(it) == name)
            return i;
    return -1;
}

int EnvironmentModel::findInResultInsertPosition(const QString &name) const
{
    Environment::const_iterator it;
    int i = 0;
    for (it = m_resultEnvironment.constBegin(); it != m_resultEnvironment.constEnd(); ++it, ++i)
        if (m_resultEnvironment.key(it) > name)
            return i;
    return m_resultEnvironment.size();
}

bool EnvironmentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole && index.isValid()) {
        if (index.column() == 0) {
            //fail if a variable with the same name already exists
#ifdef Q_OS_WIN
            if (findInChanges(value.toString().toUpper()) != -1)
                return false;
#else
            if (findInChanges(value.toString()) != -1)
                return false;
#endif
            EnvironmentItem old("", "");
            if (m_mergedEnvironments) {
                int pos = findInChanges(indexToVariable(index));
                if (pos != -1) {
                    old = m_items.at(pos);
                } else {
                    old.name = m_resultEnvironment.key(m_resultEnvironment.constBegin() + index.row());
                    old.value = m_resultEnvironment.value(m_resultEnvironment.constBegin() + index.row());
                    old.unset = false;
                }
            } else {
                old = m_items.at(index.row());
            }
#ifdef Q_OS_WIN
            const QString &newName = value.toString().toUpper();
#else
            const QString &newName = value.toString();
#endif
            if (changes(old.name))
                removeVariable(old.name);
            old.name = newName;
            addVariable(old);
            return true;
        } else if (index.column() == 1) {
            if (m_mergedEnvironments) {
                const QString &name = indexToVariable(index);
                int pos = findInChanges(name);
                if (pos != -1) {
                    m_items[pos].value = value.toString();
                    m_items[pos].unset = false;
                    updateResultEnvironment();
                    emit dataChanged(index, index);
                    emit userChangesUpdated();
                    return true;
                }
                // not found in m_items, so add it as a new variable
                addVariable(EnvironmentItem(name, value.toString()));
                return true;
            } else {
                m_items[index.row()].value = value.toString();
                m_items[index.row()].unset = false;
                emit dataChanged(index, index);
                emit userChangesUpdated();
                return true;
            }
        }
    }
    return false;
}

QModelIndex EnvironmentModel::addVariable()
{
    const QString &name = "<VARIABLE>";
    if (m_mergedEnvironments) {
        int i = findInResult(name);
        if (i != -1)
            return index(i, 0, QModelIndex());
    } else {
        int i = findInChanges(name);
        if (i != -1)
            return index(i, 0, QModelIndex());
    }
    // Don't exist, really add them
    return addVariable(EnvironmentItem(name, "<VALUE>"));
}

QModelIndex EnvironmentModel::addVariable(const EnvironmentItem &item)
{
    if (m_mergedEnvironments) {
        bool existsInBaseEnvironment = (m_baseEnvironment.find(item.name) != m_baseEnvironment.constEnd());
        int rowInResult;
        if (existsInBaseEnvironment)
            rowInResult = findInResult(item.name);
        else
            rowInResult = findInResultInsertPosition(item.name);
        int rowInChanges = findInChangesInsertPosition(item.name);

        //qDebug() << "addVariable " << item.name << existsInBaseEnvironment << rowInResult << rowInChanges;

        if (existsInBaseEnvironment) {
            m_items.insert(rowInChanges, item);
            updateResultEnvironment();
            emit dataChanged(index(rowInResult, 0, QModelIndex()), index(rowInResult, 1, QModelIndex()));
            emit userChangesUpdated();
            return index(rowInResult, 0, QModelIndex());
        } else {
            beginInsertRows(QModelIndex(), rowInResult, rowInResult);
            m_items.insert(rowInChanges, item);
            updateResultEnvironment();
            endInsertRows();
            qDebug()<<"returning index: "<<rowInResult;
            emit userChangesUpdated();
            return index(rowInResult, 0, QModelIndex());
        }
    } else {
        int newPos = findInChangesInsertPosition(item.name);
        beginInsertRows(QModelIndex(), newPos, newPos);
        m_items.insert(newPos, item);
        endInsertRows();
        emit userChangesUpdated();
        return index(newPos, 0, QModelIndex());
    }
}

void EnvironmentModel::removeVariable(const QString &name)
{
    if (m_mergedEnvironments) {
        int rowInResult = findInResult(name);
        int rowInChanges = findInChanges(name);
        bool existsInBaseEnvironment = m_baseEnvironment.find(name) != m_baseEnvironment.constEnd();
        if (existsInBaseEnvironment) {
            m_items.removeAt(rowInChanges);
            updateResultEnvironment();
            emit dataChanged(index(rowInResult, 0, QModelIndex()), index(rowInResult, 1, QModelIndex()));
            emit userChangesUpdated();
        } else {
            beginRemoveRows(QModelIndex(), rowInResult, rowInResult);
            m_items.removeAt(rowInChanges);
            updateResultEnvironment();
            endRemoveRows();
            emit userChangesUpdated();
        }
    } else {
        int removePos = findInChanges(name);
        beginRemoveRows(QModelIndex(), removePos, removePos);
        m_items.removeAt(removePos);
        updateResultEnvironment();
        endRemoveRows();
        emit userChangesUpdated();
    }
}

void EnvironmentModel::unset(const QString &name)
{
    if (m_mergedEnvironments) {
        int row = findInResult(name);
        // look in m_items for the variable
        int pos = findInChanges(name);
        if (pos != -1) {
            m_items[pos].unset = true;
            updateResultEnvironment();
            emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
            emit userChangesUpdated();
            return;
        }
        pos = findInChangesInsertPosition(name);
        m_items.insert(pos, EnvironmentItem(name, ""));
        m_items[pos].unset = true;
        updateResultEnvironment();
        emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
        emit userChangesUpdated();
        return;
    } else {
        int pos = findInChanges(name);
        m_items[pos].unset = true;
        emit dataChanged(index(pos, 1, QModelIndex()), index(pos, 1, QModelIndex()));
        emit userChangesUpdated();
        return;
    }
}

bool EnvironmentModel::isUnset(const QString &name)
{
    int pos = findInChanges(name);
    if (pos != -1)
        return m_items.at(pos).unset;
    else
        return false;
}

bool EnvironmentModel::isInBaseEnvironment(const QString &name)
{
    return m_baseEnvironment.find(name) != m_baseEnvironment.constEnd();
}

QList<EnvironmentItem> EnvironmentModel::userChanges() const
{
    return m_items;
}

void EnvironmentModel::setUserChanges(QList<EnvironmentItem> list)
{
    m_items = list;
    updateResultEnvironment();
    emit reset();
}
