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

#include "environmentmodel.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QFont>
#include <QTextEdit>

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
            if (item.unset)
                m_resultEnvironment.set(item.name, EnvironmentModel::tr("<UNSET>"));
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
    d(new Internal::EnvironmentModelPrivate)
{ }

EnvironmentModel::~EnvironmentModel()
{
    delete d;
}

QString EnvironmentModel::indexToVariable(const QModelIndex &index) const
{
    return d->m_resultEnvironment.key(d->m_resultEnvironment.constBegin() + index.row());
}

void EnvironmentModel::setBaseEnvironment(const Utils::Environment &env)
{
    if (d->m_baseEnvironment == env)
        return;
    beginResetModel();
    d->m_baseEnvironment = env;
    d->updateResultEnvironment();
    endResetModel();
}

int EnvironmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return d->m_resultEnvironment.size();
}
int EnvironmentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

bool EnvironmentModel::changes(const QString &name) const
{
    return d->findInChanges(name) >= 0;
}

QVariant EnvironmentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
        if (index.column() == 0) {
            return d->m_resultEnvironment.key(d->m_resultEnvironment.constBegin() + index.row());
        } else if (index.column() == 1) {
            // Do not return "<UNSET>" when editing a previously unset variable:
            if (role == Qt::EditRole) {
                int pos = d->findInChanges(indexToVariable(index));
                if (pos >= 0)
                    return d->m_items.at(pos).value;
            }
            QString value = d->m_resultEnvironment.value(d->m_resultEnvironment.constBegin() + index.row());
            if (role == Qt::ToolTipRole && value.length() > 80) {
                // Use html to enable text wrapping
                value = Qt::escape(value);
                value.prepend(QLatin1String("<html><body>"));
                value.append(QLatin1String("</body></html>"));
            }
            return value;
        }
    }
    if (role == Qt::FontRole) {
        // check whether this environment variable exists in d->m_items
        if (changes(d->m_resultEnvironment.key(d->m_resultEnvironment.constBegin() + index.row()))) {
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
    int row = d->findInResult(name);
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
    const QString oldValue = data(this->index(index.row(), 1, QModelIndex()), Qt::EditRole).toString();
    int changesPos = d->findInChanges(oldName);

    if (index.column() == 0) {
        //fail if a variable with the same name already exists
        const QString &newName = HostOsInfo::isWindowsHost()
                ? value.toString().toUpper() : value.toString();
        // Does the new name exist already?
        if (d->m_resultEnvironment.hasKey(newName) || newName.isEmpty())
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
            if (d->m_baseEnvironment.hasKey(oldName) && stringValue == d->m_baseEnvironment.value(oldName)) {
                // ... and now went back to the base value
                d->m_items.removeAt(changesPos);
            } else {
                // ... and changed it again
                d->m_items[changesPos].value = stringValue;
                d->m_items[changesPos].unset = false;
            }
        } else {
            // Add a new change item:
            d->m_items.append(Utils::EnvironmentItem(oldName, stringValue));
        }
        d->updateResultEnvironment();
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
    int pos = d->findInResult(item.name);
    if (pos >= 0 && pos < d->m_resultEnvironment.size())
        return index(pos, 0, QModelIndex());

    int insertPos = d->findInResultInsertPosition(item.name);
    int changePos = d->findInChanges(item.name);
    if (d->m_baseEnvironment.hasKey(item.name)) {
        // We previously unset this!
        Q_ASSERT(changePos >= 0);
        // Do not insert a line here as we listed the variable as <UNSET> before!
        Q_ASSERT(d->m_items.at(changePos).name == item.name);
        Q_ASSERT(d->m_items.at(changePos).unset);
        Q_ASSERT(d->m_items.at(changePos).value.isEmpty());
        d->m_items[changePos] = item;
        emit dataChanged(index(insertPos, 0, QModelIndex()), index(insertPos, 1, QModelIndex()));
    } else {
        // We add something that is not in the base environment
        // Insert a new line!
        beginInsertRows(QModelIndex(), insertPos, insertPos);
        Q_ASSERT(changePos < 0);
        d->m_items.append(item);
        d->updateResultEnvironment();
        endInsertRows();
    }
    emit userChangesChanged();
    return index(insertPos, 0, QModelIndex());
}

void EnvironmentModel::resetVariable(const QString &name)
{
    int rowInChanges = d->findInChanges(name);
    if (rowInChanges < 0)
        return;

    int rowInResult = d->findInResult(name);
    if (rowInResult < 0)
        return;

    if (d->m_baseEnvironment.hasKey(name)) {
        d->m_items.removeAt(rowInChanges);
        d->updateResultEnvironment();
        emit dataChanged(index(rowInResult, 0, QModelIndex()), index(rowInResult, 1, QModelIndex()));
        emit userChangesChanged();
    } else {
        // Remove the line completely!
        beginRemoveRows(QModelIndex(), rowInResult, rowInResult);
        d->m_items.removeAt(rowInChanges);
        d->updateResultEnvironment();
        endRemoveRows();
        emit userChangesChanged();
    }
}

void EnvironmentModel::unsetVariable(const QString &name)
{
    // This does not change the number of rows as we will display a <UNSET>
    // in place of the original variable!
    int row = d->findInResult(name);
    if (row < 0)
        return;

    // look in d->m_items for the variable
    int pos = d->findInChanges(name);
    if (pos != -1) {
        d->m_items[pos].unset = true;
        d->m_items[pos].value.clear();
        d->updateResultEnvironment();
        emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
        emit userChangesChanged();
        return;
    }
    Utils::EnvironmentItem item(name, QString());
    item.unset = true;
    d->m_items.append(item);
    d->updateResultEnvironment();
    emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
    emit userChangesChanged();
}

bool EnvironmentModel::canUnset(const QString &name)
{
    int pos = d->findInChanges(name);
    if (pos != -1)
        return d->m_items.at(pos).unset;
    else
        return false;
}

bool EnvironmentModel::canReset(const QString &name)
{
    return d->m_baseEnvironment.hasKey(name);
}

QList<Utils::EnvironmentItem> EnvironmentModel::userChanges() const
{
    return d->m_items;
}

void EnvironmentModel::setUserChanges(QList<Utils::EnvironmentItem> list)
{
    // We assume nobody is reordering the items here.
    if (list == d->m_items)
        return;
    beginResetModel();
    d->m_items = list;
    for (int i = 0; i != list.size(); ++i) {
        QString &name = d->m_items[i].name;
        name = name.trimmed();
        if (name.startsWith(QLatin1String("export ")))
            name = name.mid(7).trimmed();
    }

    d->updateResultEnvironment();
    endResetModel();
    emit userChangesChanged();
}

} // namespace Utils
