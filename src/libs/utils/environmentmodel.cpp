/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "environmentmodel.h"

#include <utils/environment.h>

#include <QtGui/QFont>

namespace Utils {

namespace Internal {

class EnvironmentModelPrivate
{
public:
    void updateResultEnvironment()
    {
        m_resultEnvironment = m_baseEnvironment;
        m_resultEnvironment.modify(m_items);
        // Add removed variables again and mark them as "<UNSET>" so
        // that the user can actually see those removals:
        foreach (const Utils::EnvironmentItem &item, m_items) {
            if (item.unset) {
                m_resultEnvironment.set(item.name, EnvironmentModel::tr("<UNSET>"));
            }
        }
    }

    int findInChanges(const QString &name) const
    {
        for (int i=0; i<m_items.size(); ++i)
            if (m_items.at(i).name == name)
                return i;
        return -1;
    }

    int findInResultInsertPosition(const QString &name) const
    {
        Utils::Environment::const_iterator it;
        int i = 0;
        for (it = m_resultEnvironment.constBegin(); it != m_resultEnvironment.constEnd(); ++it, ++i)
            if (m_resultEnvironment.key(it) > name)
                return i;
        return m_resultEnvironment.size();
    }

    int findInResult(const QString &name) const
    {
        Utils::Environment::const_iterator it;
        int i = 0;
        for (it = m_resultEnvironment.constBegin(); it != m_resultEnvironment.constEnd(); ++it, ++i)
            if (m_resultEnvironment.key(it) == name)
                return i;
        return -1;
    }

    Utils::Environment m_baseEnvironment;
    Utils::Environment m_resultEnvironment;
    QList<Utils::EnvironmentItem> m_items;
};

} // namespace Internal

EnvironmentModel::EnvironmentModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_d(new Internal::EnvironmentModelPrivate)
{ }

EnvironmentModel::~EnvironmentModel()
{
    delete m_d;
}

QString EnvironmentModel::indexToVariable(const QModelIndex &index) const
{
    return m_d->m_resultEnvironment.key(m_d->m_resultEnvironment.constBegin() + index.row());
}

void EnvironmentModel::setBaseEnvironment(const Utils::Environment &env)
{
    if (m_d->m_baseEnvironment == env)
        return;
    beginResetModel();
    m_d->m_baseEnvironment = env;
    m_d->updateResultEnvironment();
    endResetModel();
}

int EnvironmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_d->m_resultEnvironment.size();
}
int EnvironmentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

bool EnvironmentModel::changes(const QString &name) const
{
    return m_d->findInChanges(name) >= 0;
}

QVariant EnvironmentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
        if (index.column() == 0) {
            return m_d->m_resultEnvironment.key(m_d->m_resultEnvironment.constBegin() + index.row());
        } else if (index.column() == 1) {
            // Do not return "<UNSET>" when editing a previously unset variable:
            if (role == Qt::EditRole) {
                int pos = m_d->findInChanges(indexToVariable(index));
                if (pos >= 0)
                    return m_d->m_items.at(pos).value;
            }
            return m_d->m_resultEnvironment.value(m_d->m_resultEnvironment.constBegin() + index.row());
        }
    }
    if (role == Qt::FontRole) {
        // check whether this environment variable exists in m_d->m_items
        if (changes(m_d->m_resultEnvironment.key(m_d->m_resultEnvironment.constBegin() + index.row()))) {
            QFont f;
            f.setBold(true);
            return QVariant(f);
        }
        return QFont();
    }
    return QVariant();
}

Qt::ItemFlags EnvironmentModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

QVariant EnvironmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Variable") : tr("Value");
}

/// *****************
/// Utility functions
/// *****************
QModelIndex EnvironmentModel::variableToIndex(const QString &name) const
{
    int row = m_d->findInResult(name);
    if (row == -1)
        return QModelIndex();
    return index(row, 0);
}

bool EnvironmentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    // ignore changes to already set values:
    if (data(index, role) == value)
        return true;

    const QString oldName = data(this->index(index.row(), 0, QModelIndex())).toString();
    const QString oldValue = data(this->index(index.row(), 1, QModelIndex())).toString();
    int changesPos = m_d->findInChanges(oldName);

    if (index.column() == 0) {
        //fail if a variable with the same name already exists
#if defined(Q_OS_WIN)
        const QString &newName = value.toString().toUpper();
#else
        const QString &newName = value.toString();
#endif
        // Does the new name exist already?
        if (m_d->m_resultEnvironment.hasKey(newName))
            return false;

        Utils::EnvironmentItem newVariable(newName, oldValue);

        if (changesPos != -1)
            resetVariable(oldName); // restore the original base variable again

        QModelIndex newIndex = addVariable(newVariable); // add the new variable
        emit focusIndex(newIndex.sibling(newIndex.row(), 1)); // hint to focus on the value
        return true;
    } else if (index.column() == 1) {
        // We are changing an existing value:
        const QString stringValue = value.toString();
        if (changesPos != -1) {
            // We have already changed this value
            if (stringValue == m_d->m_baseEnvironment.value(oldName)) {
                // ... and now went back to the base value
                m_d->m_items.removeAt(changesPos);
            } else {
                // ... and changed it again
                m_d->m_items[changesPos].value = stringValue;
                m_d->m_items[changesPos].unset = false;
            }
        } else {
            // Add a new change item:
            m_d->m_items.append(Utils::EnvironmentItem(oldName, stringValue));
        }
        m_d->updateResultEnvironment();
        emit dataChanged(index, index);
        emit userChangesChanged();
        return true;
    }
    return false;
}

QModelIndex EnvironmentModel::addVariable()
{
    //: Name when inserting a new variable
    return addVariable(Utils::EnvironmentItem(tr("<VARIABLE>"),
                                              //: Value when inserting a new variable
                                              tr("<VALUE>")));
}

QModelIndex EnvironmentModel::addVariable(const Utils::EnvironmentItem &item)
{

    // Return existing index if the name is already in the result set:
    int pos = m_d->findInResult(item.name);
    if (pos >= 0 && pos < m_d->m_resultEnvironment.size())
        return index(pos, 0, QModelIndex());

    int insertPos = m_d->findInResultInsertPosition(item.name);
    int changePos = m_d->findInChanges(item.name);
    if (m_d->m_baseEnvironment.hasKey(item.name)) {
        // We previously unset this!
        Q_ASSERT(changePos >= 0);
        // Do not insert a line here as we listed the variable as <UNSET> before!
        Q_ASSERT(m_d->m_items.at(changePos).name == item.name);
        Q_ASSERT(m_d->m_items.at(changePos).unset);
        Q_ASSERT(m_d->m_items.at(changePos).value.isEmpty());
        m_d->m_items[changePos] = item;
        emit dataChanged(index(insertPos, 0, QModelIndex()), index(insertPos, 1, QModelIndex()));
    } else {
        // We add something that is not in the base environment
        // Insert a new line!
        beginInsertRows(QModelIndex(), insertPos, insertPos);
        Q_ASSERT(changePos < 0);
        m_d->m_items.append(item);
        m_d->updateResultEnvironment();
        endInsertRows();
    }
    emit userChangesChanged();
    return index(insertPos, 0, QModelIndex());
}

void EnvironmentModel::resetVariable(const QString &name)
{
    int rowInChanges = m_d->findInChanges(name);
    if (rowInChanges < 0)
        return;

    int rowInResult = m_d->findInResult(name);
    if (rowInResult < 0)
        return;

    if (m_d->m_baseEnvironment.hasKey(name)) {
        m_d->m_items.removeAt(rowInChanges);
        m_d->updateResultEnvironment();
        emit dataChanged(index(rowInResult, 0, QModelIndex()), index(rowInResult, 1, QModelIndex()));
        emit userChangesChanged();
    } else {
        // Remove the line completely!
        beginRemoveRows(QModelIndex(), rowInResult, rowInResult);
        m_d->m_items.removeAt(rowInChanges);
        m_d->updateResultEnvironment();
        endRemoveRows();
        emit userChangesChanged();
    }
}

void EnvironmentModel::unsetVariable(const QString &name)
{
    // This does not change the number of rows as we will display a <UNSET>
    // in place of the original variable!
    int row = m_d->findInResult(name);
    if (row < 0)
        return;

    // look in m_d->m_items for the variable
    int pos = m_d->findInChanges(name);
    if (pos != -1) {
        m_d->m_items[pos].unset = true;
        m_d->m_items[pos].value.clear();
        m_d->updateResultEnvironment();
        emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
        emit userChangesChanged();
        return;
    }
    Utils::EnvironmentItem item(name, QString());
    item.unset = true;
    m_d->m_items.append(item);
    m_d->updateResultEnvironment();
    emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
    emit userChangesChanged();
}

bool EnvironmentModel::canUnset(const QString &name)
{
    int pos = m_d->findInChanges(name);
    if (pos != -1)
        return m_d->m_items.at(pos).unset;
    else
        return false;
}

bool EnvironmentModel::canReset(const QString &name)
{
    return m_d->m_baseEnvironment.hasKey(name);
}

QList<Utils::EnvironmentItem> EnvironmentModel::userChanges() const
{
    return m_d->m_items;
}

void EnvironmentModel::setUserChanges(QList<Utils::EnvironmentItem> list)
{
    // We assume nobody is reordering the items here.
    if (list == m_d->m_items)
        return;
    beginResetModel();
    m_d->m_items = list;
    m_d->updateResultEnvironment();
    endResetModel();
}

} // namespace Utils
