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

#include <QAbstractTableModel>

namespace CMakeProjectManager {

class ConfigModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Roles {
        ItemTypeRole = Qt::UserRole,
        ItemValuesRole
    };

    class DataItem {
    public:
        enum Type { BOOLEAN, FILE, DIRECTORY, STRING, UNKNOWN};

        QString key;
        Type type = STRING;
        bool isAdvanced = false;
        bool inCMakeCache = false;
        QString value;
        QString description;
        QStringList values;
    };

    explicit ConfigModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void appendConfiguration(const QString &key,
                             const QString &value = QString(),
                             const DataItem::Type type = DataItem::UNKNOWN,
                             const QString &description = QString(),
                             const QStringList &values = QStringList());
    void setConfiguration(const QList<DataItem> &config);
    void setKitConfiguration(const QHash<QString, QString> &kitConfig);
    void flush();
    void resetAllChanges();

    bool hasChanges() const;
    bool hasCMakeChanges() const;

    QList<DataItem> configurationChanges() const;

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
    };

    InternalDataItem &itemAtRow(int row);
    const InternalDataItem &itemAtRow(int row) const;
    QList<InternalDataItem> m_configuration;
    QHash<QString, QString> m_kitConfiguartion;
};

} // namespace CMakeProjectManager
