// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeconfigitem.h"

#include <utils/treemodel.h>

#include <QString>

namespace CMakeProjectManager {
namespace Internal {

class ConfigModelTreeItem;

class ConfigModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    enum Roles {
        ItemIsAdvancedRole = Qt::UserRole,
        ItemIsInitialRole
    };

    struct DataItem {
        bool operator==(const DataItem &other) const {
            return key == other.key && isInitial == other.isInitial;
        }

        DataItem() {}
        DataItem(const CMakeConfigItem &cmi);

        void setType(CMakeConfigItem::Type cmt);

        QString typeDisplay() const;

        CMakeConfigItem toCMakeConfigItem() const;

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
    bool setData(const QModelIndex &idx, const QVariant &data, int role) final;

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

    using KitConfiguration = QHash<QString, CMakeConfigItem>;
    void setConfigurationFromKit(const KitConfiguration &kitConfig);

    void flush();
    void resetAllChanges(bool initialParameters = false);

    bool hasChanges(bool initialParameters = false) const;

    bool canForceTo(const QModelIndex &idx, const DataItem::Type type) const;
    void forceTo(const QModelIndex &idx, const DataItem::Type type);

    void toggleUnsetFlag(const QModelIndex &idx);

    void applyKitValue(const  QModelIndex &idx);
    void applyInitialValue(const  QModelIndex &idx);

    static DataItem dataItemFromIndex(const QModelIndex &idx);

    QList<DataItem> configurationForCMake() const;

    Utils::MacroExpander *macroExpander() const;
    void setMacroExpander(Utils::MacroExpander *newExpander);

private:
    enum class KitOrInitial { Kit, Initial };
    void applyKitOrInitialValue(const QModelIndex &idx, KitOrInitial ki);

    class InternalDataItem : public DataItem
    {
    public:
        InternalDataItem(const DataItem &item);

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
