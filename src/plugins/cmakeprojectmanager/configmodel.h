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

#include <QAbstractTableModel>
#include <utils/treemodel.h>

namespace CMakeProjectManager {

namespace Internal {  class ConfigModelTreeItem; }

class ConfigModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    enum Roles {
        ItemIsAdvancedRole = Qt::UserRole,
    };

    class DataItem {
    public:
        enum Type { BOOLEAN, FILE, DIRECTORY, STRING, UNKNOWN};

        QString key;
        Type type = STRING;
        bool isHidden = false;
        bool isAdvanced = false;
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
                             const QString &description = QString(),
                             const QStringList &values = QStringList());
    void setConfiguration(const CMakeConfig &config);
    void setConfiguration(const QList<DataItem> &config);
    void setConfigurationFromKit(const QHash<QString, QString> &kitConfig);
    void setConfigurationForCMake(const QHash<QString, QString> &config);
    void flush();
    void resetAllChanges();

    bool hasChanges() const;
    bool hasCMakeChanges() const;

    bool canForceTo(const QModelIndex &idx, const DataItem::Type type) const;
    void forceTo(const QModelIndex &idx, const DataItem::Type type);

    void toggleUnsetFlag(const QModelIndex &idx);

    static DataItem dataItemFromIndex(const QModelIndex &idx);

    QList<DataItem> configurationForCMake() const;


private:
    class InternalDataItem : public DataItem
    {
    public:
        InternalDataItem(const DataItem &item);
        InternalDataItem(const InternalDataItem &item) = default;

        QString toolTip() const;
        QString currentValue() const;

        bool isUserChanged = false;
        bool isUserNew = false;
        bool isCMakeChanged = false;
        QString newValue;
        QString kitValue;
    };

    void generateTree();

    void setConfiguration(const QList<InternalDataItem> &config);
    QList<InternalDataItem> m_configuration;
    QHash<QString, QString> m_kitConfiguration;

    friend class Internal::ConfigModelTreeItem;
};

namespace Internal {

class ConfigModelTreeItem  : public Utils::TreeItem
{
public:
    ConfigModelTreeItem(ConfigModel::InternalDataItem *di = nullptr) : dataItem(di) {}
    virtual ~ConfigModelTreeItem() override;

    QVariant data(int column, int role) const final;
    bool setData(int column, const QVariant &data, int role) final;
    Qt::ItemFlags flags(int column) const final;

    QString toolTip() const;
    QString currentValue() const;

    ConfigModel::InternalDataItem *dataItem;
};

} // namespace Internal
} // namespace CMakeProjectManager
