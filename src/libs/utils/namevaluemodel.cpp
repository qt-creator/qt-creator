/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "namevaluemodel.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/namevaluedictionary.h>
#include <utils/qtcassert.h>

#include <QFont>
#include <QGuiApplication>
#include <QPalette>
#include <QString>

namespace Utils {

namespace Internal {

class NameValueModelPrivate
{
public:
    void updateResultNameValueDictionary()
    {
        m_resultNameValueDictionary = m_baseNameValueDictionary;
        m_resultNameValueDictionary.modify(m_items);
        // Add removed variables again and mark them as "<UNSET>" so
        // that the user can actually see those removals:
        for (const NameValueItem &item : qAsConst(m_items)) {
            if (item.operation == NameValueItem::Unset)
                m_resultNameValueDictionary.set(item.name, NameValueModel::tr("<UNSET>"));
        }
    }

    int findInChanges(const QString &name) const
    {
        for (int i = 0; i < m_items.size(); ++i)
            if (m_items.at(i).name.compare(name,
                                           m_baseNameValueDictionary.nameCaseSensitivity()) == 0) {
                return i;
            }
        return -1;
    }

    int findInResultInsertPosition(const QString &name) const
    {
        NameValueDictionary::const_iterator it;
        int i = 0;
        for (it = m_resultNameValueDictionary.constBegin();
             it != m_resultNameValueDictionary.constEnd();
             ++it, ++i)
            if (it.key() > DictKey(name, m_resultNameValueDictionary.nameCaseSensitivity()))
                return i;
        return m_resultNameValueDictionary.size();
    }

    int findInResult(const QString &name) const
    {
        NameValueDictionary::const_iterator it;
        int i = 0;
        for (it = m_resultNameValueDictionary.constBegin();
             it != m_resultNameValueDictionary.constEnd();
             ++it, ++i)
            if (m_resultNameValueDictionary.key(it)
                    .compare(name, m_resultNameValueDictionary.nameCaseSensitivity()) == 0) {
                return i;
            }
        return -1;
    }

    NameValueDictionary m_baseNameValueDictionary;
    NameValueDictionary m_resultNameValueDictionary;
    NameValueItems m_items;
};

} // namespace Internal

NameValueModel::NameValueModel(QObject *parent)
    : QAbstractTableModel(parent)
    , d(std::make_unique<Internal::NameValueModelPrivate>())
{}

NameValueModel::~NameValueModel() = default;

QString NameValueModel::indexToVariable(const QModelIndex &index) const
{
    return d->m_resultNameValueDictionary.key(d->m_resultNameValueDictionary.constBegin()
                                              + index.row());
}

void NameValueModel::setBaseNameValueDictionary(const NameValueDictionary &dictionary)
{
    if (d->m_baseNameValueDictionary == dictionary)
        return;
    beginResetModel();
    d->m_baseNameValueDictionary = dictionary;
    d->updateResultNameValueDictionary();
    endResetModel();
}

int NameValueModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return d->m_resultNameValueDictionary.size();
}
int NameValueModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

bool NameValueModel::changes(const QString &name) const
{
    return d->findInChanges(name) >= 0;
}

const NameValueDictionary &NameValueModel::baseNameValueDictionary() const
{
    return d->m_baseNameValueDictionary;
}

QVariant NameValueModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto resultIterator = d->m_resultNameValueDictionary.constBegin() + index.row();
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        if (index.column() == 0)
            return d->m_resultNameValueDictionary.key(resultIterator);
        if (index.column() == 1) {
            // Do not return "<UNSET>" when editing a previously unset variable:
            if (role == Qt::EditRole) {
                int pos = d->findInChanges(indexToVariable(index));
                if (pos != -1 && d->m_items.at(pos).operation == NameValueItem::Unset)
                    return QString();
            }
            QString value = d->m_resultNameValueDictionary.value(resultIterator);
            if (role == Qt::ToolTipRole && value.length() > 80) {
                // Use html to enable text wrapping
                value = value.toHtmlEscaped();
                value.prepend(QLatin1String("<html><body>"));
                value.append(QLatin1String("</body></html>"));
            }
            return value;
        }
        break;
    case Qt::FontRole: {
        QFont f;
        f.setStrikeOut(!d->m_resultNameValueDictionary.isEnabled(resultIterator));
        return f;
    }
    case Qt::ForegroundRole: {
        const QPalette p = QGuiApplication::palette();
        return p.color(changes(d->m_resultNameValueDictionary.key(resultIterator))
                    ? QPalette::Link : QPalette::Text);
    }
    }
    return QVariant();
}

Qt::ItemFlags NameValueModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

QVariant NameValueModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    return section == 0 ? tr("Variable") : tr("Value");
}

/// *****************
/// Utility functions
/// *****************
QModelIndex NameValueModel::variableToIndex(const QString &name) const
{
    int row = d->findInResult(name);
    if (row == -1)
        return QModelIndex();
    return index(row, 0);
}

bool NameValueModel::setData(const QModelIndex &index, const QVariant &value, int role)
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
        const QString &newName = HostOsInfo::isWindowsHost() ? value.toString().toUpper()
                                                             : value.toString();
        if (newName.isEmpty() || newName.contains('='))
            return false;
        // Does the new name exist already?
        if (d->m_resultNameValueDictionary.hasKey(newName) || newName.isEmpty())
            return false;

        NameValueItem newVariable(newName, oldValue);

        if (changesPos != -1)
            resetVariable(oldName); // restore the original base variable again

        QModelIndex newIndex = addVariable(newVariable);      // add the new variable
        emit focusIndex(newIndex.sibling(newIndex.row(), 1)); // hint to focus on the value
        return true;
    } else if (index.column() == 1) {
        // We are changing an existing value:
        const QString stringValue = value.toString();
        if (changesPos != -1) {
            const auto oldIt = d->m_baseNameValueDictionary.constFind(oldName);
            const auto newIt = d->m_resultNameValueDictionary.constFind(oldName);
            // We have already changed this value
            if (oldIt != d->m_baseNameValueDictionary.constEnd()
                    && stringValue == d->m_baseNameValueDictionary.value(oldIt)
                    && d->m_baseNameValueDictionary.isEnabled(oldIt)
                            == d->m_resultNameValueDictionary.isEnabled(newIt)) {
                // ... and now went back to the base value
                d->m_items.removeAt(changesPos);
            } else {
                // ... and changed it again
                d->m_items[changesPos].value = stringValue;
                if (d->m_items[changesPos].operation == NameValueItem::Unset)
                    d->m_items[changesPos].operation = NameValueItem::SetEnabled;
            }
        } else {
            // Add a new change item:
            d->m_items.append(NameValueItem(oldName, stringValue));
        }
        d->updateResultNameValueDictionary();
        emit dataChanged(index, index);
        emit userChangesChanged();
        return true;
    }
    return false;
}

QModelIndex NameValueModel::addVariable()
{
    //: Name when inserting a new variable
    return addVariable(NameValueItem(tr("<VARIABLE>"),
                                     //: Value when inserting a new variable
                                     tr("<VALUE>")));
}

QModelIndex NameValueModel::addVariable(const NameValueItem &item)
{
    // Return existing index if the name is already in the result set:
    int pos = d->findInResult(item.name);
    if (pos >= 0 && pos < d->m_resultNameValueDictionary.size())
        return index(pos, 0, QModelIndex());

    int insertPos = d->findInResultInsertPosition(item.name);
    int changePos = d->findInChanges(item.name);
    if (d->m_baseNameValueDictionary.hasKey(item.name)) {
        // We previously unset this!
        Q_ASSERT(changePos >= 0);
        // Do not insert a line here as we listed the variable as <UNSET> before!
        Q_ASSERT(d->m_items.at(changePos).name == item.name);
        Q_ASSERT(d->m_items.at(changePos).operation == NameValueItem::Unset);
        Q_ASSERT(d->m_items.at(changePos).value.isEmpty());
        d->m_items[changePos] = item;
        emit dataChanged(index(insertPos, 0, QModelIndex()), index(insertPos, 1, QModelIndex()));
    } else {
        // We add something that is not in the base dictionary
        // Insert a new line!
        beginInsertRows(QModelIndex(), insertPos, insertPos);
        Q_ASSERT(changePos < 0);
        d->m_items.append(item);
        d->updateResultNameValueDictionary();
        endInsertRows();
    }
    emit userChangesChanged();
    return index(insertPos, 0, QModelIndex());
}

void NameValueModel::resetVariable(const QString &name)
{
    int rowInChanges = d->findInChanges(name);
    if (rowInChanges < 0)
        return;

    int rowInResult = d->findInResult(name);
    if (rowInResult < 0)
        return;

    if (d->m_baseNameValueDictionary.hasKey(name)) {
        d->m_items.removeAt(rowInChanges);
        d->updateResultNameValueDictionary();
        emit dataChanged(index(rowInResult, 0, QModelIndex()), index(rowInResult, 1, QModelIndex()));
        emit userChangesChanged();
    } else {
        // Remove the line completely!
        beginRemoveRows(QModelIndex(), rowInResult, rowInResult);
        d->m_items.removeAt(rowInChanges);
        d->updateResultNameValueDictionary();
        endRemoveRows();
        emit userChangesChanged();
    }
}

void NameValueModel::unsetVariable(const QString &name)
{
    // This does not change the number of rows as we will display a <UNSET>
    // in place of the original variable!
    int row = d->findInResult(name);
    if (row < 0)
        return;

    // look in d->m_items for the variable
    int pos = d->findInChanges(name);
    if (pos != -1) {
        d->m_items[pos].operation = NameValueItem::Unset;
        d->m_items[pos].value.clear();
        d->updateResultNameValueDictionary();
        emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
        emit userChangesChanged();
        return;
    }
    d->m_items.append(NameValueItem(name, QString(), NameValueItem::Unset));
    d->updateResultNameValueDictionary();
    emit dataChanged(index(row, 0, QModelIndex()), index(row, 1, QModelIndex()));
    emit userChangesChanged();
}

void NameValueModel::toggleVariable(const QModelIndex &idx)
{
    const QString name = indexToVariable(idx);
    const auto newIt = d->m_resultNameValueDictionary.constFind(name);
    QTC_ASSERT(newIt != d->m_resultNameValueDictionary.constEnd(), return);
    const auto op = d->m_resultNameValueDictionary.isEnabled(newIt)
            ? NameValueItem::SetDisabled : NameValueItem::SetEnabled;
    const int changesPos = d->findInChanges(name);
    if (changesPos != -1) {
        const auto oldIt = d->m_baseNameValueDictionary.constFind(name);
        if (oldIt == d->m_baseNameValueDictionary.constEnd()
                || oldIt.value().first != newIt.value().first) {
            d->m_items[changesPos].operation = op;
        } else {
            d->m_items.removeAt(changesPos);
        }
    } else {
        d->m_items.append({name, d->m_resultNameValueDictionary.value(newIt), op});
    }
    d->updateResultNameValueDictionary();
    emit dataChanged(index(idx.row(), 0), index(idx.row(), 1));
    emit userChangesChanged();
}

bool NameValueModel::isUnset(const QString &name)
{
    const int pos = d->findInChanges(name);
    return pos == -1 ? false : d->m_items.at(pos).operation == NameValueItem::Unset;
}

bool NameValueModel::isEnabled(const QString &name) const
{
    return d->m_resultNameValueDictionary.isEnabled(d->m_resultNameValueDictionary.constFind(name));
}

bool NameValueModel::canReset(const QString &name)
{
    return d->m_baseNameValueDictionary.hasKey(name);
}

NameValueItems NameValueModel::userChanges() const
{
    return d->m_items;
}

void NameValueModel::setUserChanges(const NameValueItems &items)
{
    NameValueItems filtered = Utils::filtered(items, [](const NameValueItem &i) {
        return i.name != "export " && !i.name.contains('=');
    });
    // We assume nobody is reordering the items here.
    if (filtered == d->m_items)
        return;
    beginResetModel();
    d->m_items = filtered;
    for (NameValueItem &item : d->m_items) {
        QString &name = item.name;
        name = name.trimmed();
        if (name.startsWith("export "))
            name = name.mid(7).trimmed();
        if (d->m_baseNameValueDictionary.osType() == OsTypeWindows) {
            // NameValueDictionary variable names are case-insensitive under windows, but we still
            // want to preserve the case of pre-existing variables.
            auto it = d->m_baseNameValueDictionary.constFind(name);
            if (it != d->m_baseNameValueDictionary.constEnd())
                name = d->m_baseNameValueDictionary.key(it);
        }
    }

    d->updateResultNameValueDictionary();
    endResetModel();
    emit userChangesChanged();
}

} // namespace Utils
