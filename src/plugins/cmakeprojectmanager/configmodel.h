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

#pragma once

#include "cmakeconfigitem.h"

#include <utils/treemodel.h>

#include <QString>

namespace CMakeProjectManager {

namespace Internal {  class ConfigModelTreeItem; }

class ConfigModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    enum Roles {
        ItemIsAdvancedRole = Qt::UserRole,
        ItemIsInitialRole
    };

    struct DataItem {
        bool operator == (const DataItem& other) const {
            return key == other.key && isInitial == other.isInitial;
        }

        DataItem() {}
        DataItem(const CMakeConfigItem &cmi) {
            key = QString::fromUtf8(cmi.key);
            value = QString::fromUtf8(cmi.value);
            description = QString::fromUtf8(cmi.documentation);
            values = cmi.values;
            inCMakeCache = cmi.inCMakeCache;

            isAdvanced = cmi.isAdvanced;
            isInitial = cmi.isInitial;
            isHidden = cmi.type == CMakeConfigItem::INTERNAL || cmi.type == CMakeConfigItem::STATIC;

            setType(cmi.type);
        }

        void setType(CMakeConfigItem::Type cmt) {
            switch (cmt) {
            case CMakeConfigItem::FILEPATH:
                type = FILE;
                break;
            case CMakeConfigItem::PATH:
                type = DIRECTORY;
                break;
            case CMakeConfigItem::BOOL:
                type = BOOLEAN;
                break;
            case CMakeConfigItem::STRING:
                type = STRING;
                break;
            default:
                type = UNKNOWN;
                break;
            }
        }

        CMakeConfigItem toCMakeConfigItem() const {
            CMakeConfigItem cmi;
            cmi.key = key.toUtf8();
            cmi.value = value.toUtf8();
            switch (type) {
                case DataItem::BOOLEAN:
                    cmi.type = CMakeConfigItem::BOOL;
                    break;
                case DataItem::FILE:
                    cmi.type = CMakeConfigItem::FILEPATH;
                    break;
                case DataItem::DIRECTORY:
                    cmi.type = CMakeConfigItem::PATH;
                    break;
                case DataItem::STRING:
                    cmi.type = CMakeConfigItem::STRING;
                    break;
                case DataItem::UNKNOWN:
                    cmi.type = CMakeConfigItem::UNINITIALIZED;
                    break;
            }
            cmi.isUnset = isUnset;
            cmi.isAdvanced = isAdvanced;
            cmi.isInitial = isInitial;
            cmi.values = values;
            cmi.documentation = description.toUtf8();

            return cmi;
        }

        enum Type { BOOLEAN, FILE, DIRECTORY, STRING, UNKNOWN};

        QString key;
        Type type = STRING;
        bool isHidden = false;
        bool isAdvanced = false;
        bool isInitial = false;
        bool inCMakeCache = false;
        bool isUnset = false;
        QString value;
        QString description;
        QStringList values;
    };

    explicit ConfigModel(QObject *parent = nullptr);
    ~ConfigModel() override;

    QVariant data(const QModelIndex &idx, int role) const final;

    void appendConfiguration(const QString &key,
                             const QString &value = QString(),
                             const DataItem::Type type = DataItem::UNKNOWN,
                             bool isInitial = false,
                             const QString &description = QString(),
                             const QStringList &values = QStringList());
    void setConfiguration(const CMakeConfig &config);
    void setBatchEditConfiguration(const CMakeConfig &config);
    void setInitialParametersConfiguration(const CMakeConfig &config);
    void setConfiguration(const QList<DataItem> &config);

    using KitConfiguration = QHash<QString, QPair<QString,QString>>;
    void setConfigurationFromKit(const KitConfiguration &kitConfig);

    void flush();
    void resetAllChanges(bool initialParameters = false);

    bool hasChanges(bool initialParameters = false) const;

    bool canForceTo(const QModelIndex &idx, const DataItem::Type type) const;
    void forceTo(const QModelIndex &idx, const DataItem::Type type);

    void toggleUnsetFlag(const QModelIndex &idx);

    static DataItem dataItemFromIndex(const QModelIndex &idx);

    QList<DataItem> configurationForCMake() const;

    Utils::MacroExpander *macroExpander() const;
    void setMacroExpander(Utils::MacroExpander *newExpander);

private:
    class InternalDataItem : public DataItem
    {
    public:
        InternalDataItem(const DataItem &item);
        InternalDataItem(const InternalDataItem &item) = default;

        QString currentValue() const;

        bool isUserChanged = false;
        bool isUserNew = false;
        QString newValue;
        QString kitValue;
        QString initialValue;
    };

    void generateTree();

    void setConfiguration(const QList<InternalDataItem> &config);
    QList<InternalDataItem> m_configuration;
    KitConfiguration m_kitConfiguration;
    Utils::MacroExpander *m_macroExpander = nullptr;

    friend class Internal::ConfigModelTreeItem;
};

namespace Internal {

class ConfigModelTreeItem  : public Utils::TreeItem
{
public:
    ConfigModelTreeItem(ConfigModel::InternalDataItem *di = nullptr) : dataItem(di) {}
    ~ConfigModelTreeItem() override;

    QVariant data(int column, int role) const final;
    bool setData(int column, const QVariant &data, int role) final;
    Qt::ItemFlags flags(int column) const final;

    QString toolTip() const;
    QString currentValue() const;

    ConfigModel::InternalDataItem *dataItem;
};

} // namespace Internal
} // namespace CMakeProjectManager
