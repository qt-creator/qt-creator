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

#include "configmodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QCoreApplication>
#include <QFont>

namespace CMakeProjectManager {

static bool isTrue(const QString &value)
{
    const QString lower = value.toLower();
    return lower == QStringLiteral("true") || lower == QStringLiteral("on")
            || lower == QStringLiteral("1") || lower == QStringLiteral("yes");
}

ConfigModel::ConfigModel(QObject *parent) : QAbstractTableModel(parent)
{ }

int ConfigModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_configuration.count();
}

int ConfigModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 3;
}

Qt::ItemFlags ConfigModel::flags(const QModelIndex &index) const
{
    QTC_ASSERT(index.model() == this, return Qt::NoItemFlags);
    QTC_ASSERT(index.isValid(), return Qt::NoItemFlags);
    QTC_ASSERT(index.column() >= 0 && index.column() < columnCount(QModelIndex()), return Qt::NoItemFlags);
    QTC_ASSERT(index.row() >= 0 && index.row() < rowCount(QModelIndex()), return Qt::NoItemFlags);

    const InternalDataItem &item = itemAtRow(index.row());

    if (index.column() == 1) {
        if (item.type == DataItem::BOOLEAN)
            return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
        else
            return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
    } else {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (item.isUserNew)
            return flags |= Qt::ItemIsEditable;
        return flags;
    }
}

QVariant ConfigModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.model() == this, return QVariant());
    QTC_ASSERT(index.isValid(), return QVariant());
    QTC_ASSERT(index.row() >= 0 && index.row() < rowCount(QModelIndex()), return QVariant());
    QTC_ASSERT(index.column() >= 0 && index.column() < columnCount(QModelIndex()), return QVariant());

    const InternalDataItem &item = m_configuration[index.row()];

    if (index.column() < 2) {
        switch (role) {
        case ItemTypeRole:
            return item.type;
        case ItemValuesRole:
            return item.values;
        }
    }

    switch (index.column()) {
    case 0:
        switch (role) {
        case Qt::DisplayRole:
            return item.key.isEmpty() ? tr("<UNSET>") : item.key;
        case Qt::EditRole:
            return item.key;
        case Qt::ToolTipRole:
            return item.toolTip();
        case Qt::FontRole: {
            QFont font;
            font.setItalic(item.isCMakeChanged);
            font.setBold(item.isUserNew);
            font.setStrikeOut(!item.inCMakeCache && !item.isUserNew);
            return font;
        }
        default:
            return QVariant();
        }
    case 1: {
        const QString value = item.currentValue();
        const QString kitValue = m_kitConfiguartion.value(item.key);

        switch (role) {
        case Qt::CheckStateRole:
            return (item.type == DataItem::BOOLEAN)
                    ? QVariant(isTrue(value) ? Qt::Checked : Qt::Unchecked) : QVariant();
        case Qt::DisplayRole:
            return value;
        case Qt::EditRole:
            return (item.type == DataItem::BOOLEAN) ? QVariant(isTrue(value)) : QVariant(value);
        case Qt::FontRole: {
            QFont font;
            font.setBold(item.isUserChanged || item.isUserNew);
            font.setItalic(item.isCMakeChanged);
            return font;
        }
        case Qt::ForegroundRole:
            return Utils::creatorTheme()->color((!kitValue.isNull() && kitValue != value)
                    ? Utils::Theme::TextColorHighlight : Utils::Theme::TextColorNormal);
        case Qt::ToolTipRole: {
            QString tooltip = item.toolTip();
            const QString kitValue = m_kitConfiguartion.value(item.key);
            if (!kitValue.isNull()) {
                if (!tooltip.isEmpty())
                    tooltip.append("<br>");
                tooltip.append(tr("Kit value: %1").arg(kitValue));
            }
            return tooltip;
        }
        default:
            return QVariant();
        }
    }
    case 2:
        switch (role) {
        case Qt::EditRole:
            return "0";
        case Qt::DisplayRole:
            return QString::fromLatin1(item.isAdvanced ? "1" : "0");
        case Qt::CheckStateRole:
            return item.isAdvanced ? Qt::Checked : Qt::Unchecked;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

bool ConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QTC_ASSERT(index.model() == this, return false);
    QTC_ASSERT(index.isValid(), return false);
    QTC_ASSERT(index.row() >= 0 && index.row() < rowCount(QModelIndex()), return false);
    QTC_ASSERT(index.column() >= 0 && index.column() < 2, return false);

    QString newValue = value.toString();
    if (role == Qt::CheckStateRole) {
        if (index.column() != 1)
            return false;
        newValue = QString::fromLatin1(value.toInt() == 0 ? "OFF" : "ON");
    } else if (role != Qt::EditRole) {
        return false;
    }

    InternalDataItem &item = itemAtRow(index.row());
    switch (index.column()) {
    case 0:
        if (!item.key.isEmpty() && !item.isUserNew)
            return false;
        item.key = newValue;
        item.isUserNew = true;
        item.isUserChanged = false;
        emit dataChanged(index, index);
        return true;
    case 1:
        if (item.value == newValue) {
            item.newValue.clear();
            item.isUserChanged = false;
        } else {
            item.newValue = newValue;
            item.isUserChanged = true;
        }
        emit dataChanged(index, index);
        return true;
    case 2:
    default:
        return false;
    }
}

QVariant ConfigModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();
    switch (section) {
    case 0:
        return tr("Setting");
    case 1:
        return tr("Value");
    case 2:
        return tr("Advanced");
    default:
        return QVariant();
    }
}

void ConfigModel::appendConfiguration(const QString &key,
                                      const QString &value,
                                      const ConfigModel::DataItem::Type type,
                                      const QString &description,
                                      const QStringList &values)
{
    DataItem item;
    item.key = key;
    item.type = type;
    item.value = value;
    item.description = description;
    item.values = values;

    InternalDataItem internalItem(item);
    internalItem.isUserNew = true;

    beginResetModel();
    m_configuration.append(internalItem);
    endResetModel();
}

void ConfigModel::setConfiguration(const QList<ConfigModel::DataItem> &config)
{
    QList<DataItem> tmp = config;
    Utils::sort(tmp,
                [](const ConfigModel::DataItem &i, const ConfigModel::DataItem &j) {
                    return i.key < j.key;
                });
    auto newIt = tmp.constBegin();
    auto newEndIt = tmp.constEnd();
    auto oldIt = m_configuration.constBegin();
    auto oldEndIt = m_configuration.constEnd();

    QList<InternalDataItem> result;
    while (newIt != newEndIt && oldIt != oldEndIt) {
        if (newIt->key < oldIt->key) {
            // Add new entry:
            result << InternalDataItem(*newIt);
            ++newIt;
        } else if (newIt->key > oldIt->key) {
            // Keep old user settings, but skip other entries:
            if (oldIt->isUserChanged || oldIt->isUserNew)
                result << InternalDataItem(*oldIt);
            ++oldIt;
        } else {
            // merge old/new entry:
            InternalDataItem item(*newIt);
            item.newValue = (newIt->value != oldIt->newValue) ? oldIt->newValue : QString();
            item.isCMakeChanged = (oldIt->value != newIt->value);
            item.isUserChanged = !item.newValue.isEmpty() && (item.newValue != item.value);
            result << item;
            ++newIt;
            ++oldIt;
        }
    }

    // Add remaining new entries:
    for (; newIt != newEndIt; ++newIt)
        result << InternalDataItem(*newIt);

    beginResetModel();
    m_configuration = result;
    endResetModel();
}

void ConfigModel::setKitConfiguration(const QHash<QString, QString> &kitConfig)
{
    m_kitConfiguartion = kitConfig;
}

void ConfigModel::flush()
{
    beginResetModel();
    m_configuration.clear();
    endResetModel();
}

void ConfigModel::resetAllChanges()
{
    const QList<InternalDataItem> tmp
            = Utils::filtered(m_configuration,
                              [](const InternalDataItem &i) { return !i.isUserNew; });

    beginResetModel();
    m_configuration = Utils::transform(tmp, [](const InternalDataItem &i) -> InternalDataItem {
        InternalDataItem ni(i);
        ni.newValue.clear();
        ni.isUserChanged = false;
        return ni;
    });
    endResetModel();
}

bool ConfigModel::hasChanges() const
{
    return Utils::contains(m_configuration, [](const InternalDataItem &i) { return i.isUserChanged || i.isUserNew; });
}

bool ConfigModel::hasCMakeChanges() const
{
    return Utils::contains(m_configuration, [](const InternalDataItem &i) { return i.isCMakeChanged; });
}

QList<ConfigModel::DataItem> ConfigModel::configurationChanges() const
{
    const QList<InternalDataItem> tmp
            = Utils::filtered(m_configuration, [](const InternalDataItem &i) {
        return i.isUserChanged || i.isUserNew || !i.inCMakeCache;
    });
    return Utils::transform(tmp, [](const InternalDataItem &item) {
        DataItem newItem(item);
        if (item.isUserChanged)
            newItem.value = item.newValue;
        return newItem;
    });
}

ConfigModel::InternalDataItem &ConfigModel::itemAtRow(int row)
{
    QTC_CHECK(row >= 0);
    return m_configuration[row];
}

const ConfigModel::InternalDataItem &ConfigModel::itemAtRow(int row) const
{
    QTC_CHECK(row >= 0);
    return m_configuration[row];
}

ConfigModel::InternalDataItem::InternalDataItem(const ConfigModel::DataItem &item) : DataItem(item)
{ }

QString ConfigModel::InternalDataItem::toolTip() const
{
    QStringList tooltip(description);
    if (inCMakeCache) {
        if (value != newValue)
            tooltip << QCoreApplication::translate("CMakeProjectManager", "Current CMake: %1").arg(value);
    }  else {
        tooltip << QCoreApplication::translate("CMakeProjectManager", "Not in CMakeCache.txt").arg(value);
    }
    return tooltip.join("<br>");
}

QString ConfigModel::InternalDataItem::currentValue() const
{
    return isUserChanged ? newValue : value;
}

} // namespace CMakeProjectManager
