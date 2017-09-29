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

#include <utils/asconst.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QCoreApplication>
#include <QFont>
#include <QString>
#include <QSortFilterProxyModel>

namespace CMakeProjectManager {

static bool isTrue(const QString &value)
{
    const QString lower = value.toLower();
    return lower == QStringLiteral("true") || lower == QStringLiteral("on")
            || lower == QStringLiteral("1") || lower == QStringLiteral("yes");
}

ConfigModel::ConfigModel(QObject *parent) : Utils::TreeModel<>(parent)
{
    setHeader({tr("Key"), tr("Value")});
}

QVariant ConfigModel::data(const QModelIndex &idx, int role) const
{
    // Hide/show groups according to "isAdvanced" setting:
    Utils::TreeItem *item = static_cast<Utils::TreeItem *>(idx.internalPointer());
    if (role == ItemIsAdvancedRole && item->childCount() > 0) {
        const bool hasNormalChildren = item->findAnyChild([](const Utils::TreeItem *ti) {
            if (auto cmti = dynamic_cast<const Internal::ConfigModelTreeItem*>(ti))
                return !cmti->dataItem->isAdvanced;
            return false;
        }) != nullptr;
        return hasNormalChildren ? "0" : "1";
    }
    return Utils::TreeModel<>::data(idx, role);
}

ConfigModel::~ConfigModel() = default;

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

    if (m_kitConfiguration.contains(key))
        internalItem.kitValue = m_kitConfiguration.value(key);

    m_configuration.append(internalItem);
    setConfiguration(m_configuration);
}

void ConfigModel::setConfiguration(const QList<DataItem> &config)
{
    setConfiguration(Utils::transform(config, [](const DataItem &di) { return InternalDataItem(di); }));
}

void ConfigModel::setConfigurationFromKit(const QHash<QString, QString> &kitConfig)
{
    m_kitConfiguration = kitConfig;

    for (InternalDataItem &i : m_configuration) {
        if (m_kitConfiguration.contains(i.key))
            i.kitValue = m_kitConfiguration.value(i.key);
    }
    setConfiguration(m_configuration);
}

void ConfigModel::setConfigurationForCMake(const QHash<QString, QString> &config)
{
    for (InternalDataItem &i : m_configuration) {
        if (!config.contains(i.key))
            continue;

        const QString v = config.value(i.key);
        if (i.value == v) {
            i.newValue.clear();
            i.isUserChanged = false;
        } else {
            i.newValue = v;
            i.isUserChanged = true;
        }
    }
    setConfiguration(m_configuration);
}

void ConfigModel::flush()
{
    setConfiguration(QList<InternalDataItem>());
}

void ConfigModel::resetAllChanges()
{
    const QList<InternalDataItem> tmp
            = Utils::filtered(m_configuration,
                              [](const InternalDataItem &i) { return !i.isUserNew; });

    setConfiguration(Utils::transform(tmp, [](const InternalDataItem &i) {
        InternalDataItem ni(i);
        ni.newValue.clear();
        ni.isUserChanged = false;
        ni.isUnset = false;
        return ni;
    }));
}

bool ConfigModel::hasChanges() const
{
    return Utils::contains(m_configuration, [](const InternalDataItem &i) {
        return i.isUserChanged || i.isUserNew || i.isUnset;
    });
}

bool ConfigModel::hasCMakeChanges() const
{
    return Utils::contains(m_configuration, [](const InternalDataItem &i) { return i.isCMakeChanged; });
}

bool ConfigModel::canForceTo(const QModelIndex &idx, const ConfigModel::DataItem::Type type) const
{
    if (idx.model() != const_cast<ConfigModel *>(this) || idx.column() != 1)
        return false;
    Utils::TreeItem *item = itemForIndex(idx);
    auto cmti = dynamic_cast<Internal::ConfigModelTreeItem *>(item);
    return cmti && (cmti->dataItem->type != type);
}

void ConfigModel::forceTo(const QModelIndex &idx, const ConfigModel::DataItem::Type type)
{
    QTC_ASSERT(canForceTo(idx, type), return);
    Utils::TreeItem *item = itemForIndex(idx);
    auto cmti = dynamic_cast<Internal::ConfigModelTreeItem *>(item);

    cmti->dataItem->type = type;
    const QModelIndex valueIdx = idx.sibling(idx.row(), 1);
    emit dataChanged(valueIdx, valueIdx);
}

void ConfigModel::toggleUnsetFlag(const QModelIndex &idx)
{
    Utils::TreeItem *item = itemForIndex(idx);
    auto cmti = dynamic_cast<Internal::ConfigModelTreeItem *>(item);

    QTC_ASSERT(cmti, return);

    cmti->dataItem->isUnset = !cmti->dataItem->isUnset;
    const QModelIndex valueIdx = idx.sibling(idx.row(), 1);
    const QModelIndex keyIdx = idx.sibling(idx.row(), 0);
    emit dataChanged(keyIdx, valueIdx);
}

ConfigModel::DataItem ConfigModel::dataItemFromIndex(const QModelIndex &idx)
{
    const QAbstractItemModel *m = idx.model();
    QModelIndex mIdx = idx;
    while (auto sfpm = qobject_cast<const QSortFilterProxyModel *>(m)) {
        m = sfpm->sourceModel();
        mIdx = sfpm->mapToSource(mIdx);
    }
    auto model = qobject_cast<const ConfigModel *>(m);
    QTC_ASSERT(model, return DataItem());
    const QModelIndex modelIdx = mIdx;

    Utils::TreeItem *item = model->itemForIndex(modelIdx);
    auto cmti = dynamic_cast<Internal::ConfigModelTreeItem *>(item);

    if (cmti && cmti->dataItem) {
        DataItem di;
        di.key = cmti->dataItem->key;
        di.type = cmti->dataItem->type;
        di.isHidden = cmti->dataItem->isHidden;
        di.isAdvanced = cmti->dataItem->isAdvanced;
        di.inCMakeCache = cmti->dataItem->inCMakeCache;
        di.value = cmti->dataItem->currentValue();
        di.description = cmti->dataItem->description;
        di.values = cmti->dataItem->values;

        return di;
    }
    return DataItem();
}

QList<ConfigModel::DataItem> ConfigModel::configurationForCMake() const
{
    const QList<InternalDataItem> tmp
            = Utils::filtered(m_configuration, [](const InternalDataItem &i) {
        return i.isUserChanged || i.isUserNew || !i.inCMakeCache || i.isUnset;
    });
    return Utils::transform(tmp, [](const InternalDataItem &item) {
        DataItem newItem(item);
        if (item.isUserChanged)
            newItem.value = item.newValue;
        return newItem;
    });
}

void ConfigModel::setConfiguration(const CMakeConfig &config)
{
    setConfiguration(Utils::transform(config, [](const CMakeConfigItem &i) {
        ConfigModel::DataItem j;
        j.key = QString::fromUtf8(i.key);
        j.value = QString::fromUtf8(i.value);
        j.description = QString::fromUtf8(i.documentation);
        j.values = i.values;
        j.inCMakeCache = i.inCMakeCache;

        j.isAdvanced = i.isAdvanced;
        j.isHidden = i.type == CMakeConfigItem::INTERNAL || i.type == CMakeConfigItem::STATIC;

        switch (i.type) {
        case CMakeConfigItem::FILEPATH:
            j.type = ConfigModel::DataItem::FILE;
            break;
        case CMakeConfigItem::PATH:
            j.type = ConfigModel::DataItem::DIRECTORY;
            break;
        case CMakeConfigItem::BOOL:
            j.type = ConfigModel::DataItem::BOOLEAN;
            break;
        case CMakeConfigItem::STRING:
            j.type = ConfigModel::DataItem::STRING;
            break;
        default:
            j.type = ConfigModel::DataItem::UNKNOWN;
            break;
        }

        return j;
    }));
}


void ConfigModel::setConfiguration(const QList<ConfigModel::InternalDataItem> &config)
{
    QList<InternalDataItem> tmp = config;
    auto newIt = tmp.constBegin();
    auto newEndIt = tmp.constEnd();
    auto oldIt = m_configuration.constBegin();
    auto oldEndIt = m_configuration.constEnd();

    QList<InternalDataItem> result;
    while (newIt != newEndIt && oldIt != oldEndIt) {
        if (oldIt->isUnset) {
            ++oldIt;
        } else if (newIt->isHidden || newIt->isUnset) {
            ++newIt;
        } else if (newIt->key < oldIt->key) {
            // Add new entry:
            result << *newIt;
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
    for (; newIt != newEndIt; ++newIt) {
        if (newIt->isHidden)
            continue;
        result << InternalDataItem(*newIt);
    }

    m_configuration = result;

    generateTree();
}

static QString prefix(const QString &key)
{
    int pos = key.indexOf('_');
    if (pos > 0)
        return key.left(pos);
    return key;
}

void ConfigModel::generateTree()
{
    QList<QString> prefixList;

    // Generate nodes for *all* prefixes
    QHash<QString, QList<Utils::TreeItem *>> prefixes;
    for (const InternalDataItem &di : m_configuration) {
        const QString p = prefix(di.key);
        if (!prefixes.contains(p)) {
            prefixes.insert(p, {});
            prefixList.append(p);
        }
    }

    // Fill prefix nodes:
    for (InternalDataItem &di : m_configuration)
        prefixes[prefix(di.key)].append(new Internal::ConfigModelTreeItem(&di));

    Utils::TreeItem *root = new Utils::TreeItem;

    for (const QString &p : Utils::asConst(prefixList)) {
        const QList<Utils::TreeItem *> &prefixItemList = prefixes.value(p);
        QTC_ASSERT(!prefixItemList.isEmpty(), continue);

        if (prefixItemList.count() == 1) {
            root->appendChild(prefixItemList.at(0));
        } else {
            Utils::TreeItem *sti = new Utils::StaticTreeItem(p);
            for (Utils::TreeItem *const ti : prefixItemList)
                sti->appendChild(ti);
            root->appendChild(sti);
        }
        prefixes.remove(p);
    }
    QTC_CHECK(prefixes.isEmpty());

    setRootItem(root);
}

ConfigModel::InternalDataItem::InternalDataItem(const ConfigModel::DataItem &item) : DataItem(item)
{ }

QString ConfigModel::InternalDataItem::toolTip() const
{
    QString desc = description;
    if (isAdvanced)
        desc += QCoreApplication::translate("CMakeProjectManager::ConfigModel", " (ADVANCED)");
    QStringList tooltip(desc);
    if (inCMakeCache) {
        if (value != newValue)
            tooltip << QCoreApplication::translate("CMakeProjectManager", "Current CMake: %1").arg(value);
    }  else {
        tooltip << QCoreApplication::translate("CMakeProjectManager", "Not in CMakeCache.txt").arg(value);
    }
    if (!kitValue.isEmpty())
        tooltip << QCoreApplication::translate("CMakeProjectManager::ConfigModel", "Current kit: %1").arg(kitValue);
    return tooltip.join("<br>");
}

QString ConfigModel::InternalDataItem::currentValue() const
{
    if (isUnset)
        return value;
    return isUserChanged ? newValue : value;
}

namespace Internal {

ConfigModelTreeItem::~ConfigModelTreeItem() = default;

QVariant ConfigModelTreeItem::data(int column, int role) const
{
    QTC_ASSERT(column >= 0 && column < 2, return QVariant());

    QTC_ASSERT(dataItem, return QVariant());

    if (firstChild()) {
        // Node with children: Only ever show name:
        if (column == 0)
            return dataItem->key;
        return QVariant();
    }

    // Leaf node:
    if (role == ConfigModel::ItemIsAdvancedRole)
        return dataItem->isAdvanced ? "1" : "0";

    switch (column) {
    case 0:
        switch (role) {
        case Qt::DisplayRole:
            return dataItem->key.isEmpty() ? QCoreApplication::translate("CMakeProjectManager::ConfigModel", "<UNSET>") : dataItem->key;
        case Qt::EditRole:
            return dataItem->key;
        case Qt::ToolTipRole:
            return toolTip();
        case Qt::FontRole: {
            QFont font;
            font.setItalic(dataItem->isCMakeChanged);
            font.setBold(dataItem->isUserNew);
            font.setStrikeOut((!dataItem->inCMakeCache && !dataItem->isUserNew) || dataItem->isUnset);
            return font;
        }
        default:
            return QVariant();
        }
    case 1: {
        const QString value = currentValue();

        switch (role) {
        case Qt::CheckStateRole:
            return (dataItem->type == ConfigModel::DataItem::BOOLEAN)
                    ? QVariant(isTrue(value) ? Qt::Checked : Qt::Unchecked) : QVariant();
        case Qt::DisplayRole:
            return value;
        case Qt::EditRole:
            return (dataItem->type == ConfigModel::DataItem::BOOLEAN) ? QVariant(isTrue(value)) : QVariant(value);
        case Qt::FontRole: {
            QFont font;
            font.setBold((dataItem->isUserChanged || dataItem->isUserNew) && !dataItem->isUnset);
            font.setItalic(dataItem->isCMakeChanged);
            font.setStrikeOut((!dataItem->inCMakeCache && !dataItem->isUserNew) || dataItem->isUnset);
            return font;
        }
        case Qt::ForegroundRole:
            return Utils::creatorTheme()->color((!dataItem->kitValue.isNull() && dataItem->kitValue != value)
                    ? Utils::Theme::TextColorHighlight : Utils::Theme::TextColorNormal);
        case Qt::ToolTipRole: {
            return toolTip();
        }
        default:
            return QVariant();
        }
    }
    default:
        return QVariant();
    }
}

bool ConfigModelTreeItem::setData(int column, const QVariant &value, int role)
{
    QTC_ASSERT(column >= 0 && column < 2, return false);
    QTC_ASSERT(dataItem, return false);
    if (dataItem->isUnset)
        return false;

    QString newValue = value.toString();
    if (role == Qt::CheckStateRole) {
        if (column != 1)
            return false;
        newValue = QString::fromLatin1(value.toInt() == 0 ? "OFF" : "ON");
    } else if (role != Qt::EditRole) {
        return false;
    }

    switch (column) {
    case 0:
        if (!dataItem->key.isEmpty() && !dataItem->isUserNew)
            return false;
        dataItem->key = newValue;
        dataItem->isUserNew = true;
        return true;
    case 1:
        if (dataItem->value == newValue) {
            dataItem->newValue.clear();
            dataItem->isUserChanged = false;
        } else {
            dataItem->newValue = newValue;
            dataItem->isUserChanged = true;
        }
        return true;
    default:
        return false;
    }
}

Qt::ItemFlags ConfigModelTreeItem::flags(int column) const
{
    if (column < 0 || column >= 2)
        return Qt::NoItemFlags;

    QTC_ASSERT(dataItem, return Qt::NoItemFlags);

    if (dataItem->isUnset)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (column == 1) {
        if (dataItem->type == ConfigModel::DataItem::BOOLEAN)
            return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
        else
            return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
    } else {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (dataItem->isUserNew)
            return flags |= Qt::ItemIsEditable;
        return flags;
    }
}

QString ConfigModelTreeItem::toolTip() const
{
    QTC_ASSERT(dataItem, return QString());
    QStringList tooltip(dataItem->description);
    if (!dataItem->kitValue.isEmpty())
        tooltip << QCoreApplication::translate("CMakeProjectManager", "Value requested by kit: %1").arg(dataItem->kitValue);
    if (dataItem->inCMakeCache) {
        if (dataItem->value != dataItem->newValue)
            tooltip << QCoreApplication::translate("CMakeProjectManager", "Current CMake: %1").arg(dataItem->value);
    }  else {
        tooltip << QCoreApplication::translate("CMakeProjectManager", "Not in CMakeCache.txt");
    }
    return tooltip.join("<br>");
}

QString ConfigModelTreeItem::currentValue() const
{
    QTC_ASSERT(dataItem, return QString());
    return dataItem->isUserChanged ? dataItem->newValue : dataItem->value;
}

} // namespace Internal
} // namespace CMakeProjectManager
