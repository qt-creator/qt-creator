// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QPointer>

class TableHeaderLengthModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(
        QAbstractItemModel *sourceModel
        READ sourceModel
        WRITE setSourceModel
        NOTIFY sourceModelChanged)

    Q_PROPERTY(
        Qt::Orientation orientation
        READ orientation
        WRITE setOrientation
        NOTIFY orientationChanged)

    Q_PROPERTY(
        int defaultLength
        READ defaultLength
        WRITE setDefaultLength
        NOTIFY defaultLengthChanged
        FINAL)

public:
    explicit TableHeaderLengthModel(QObject *parent = nullptr);
    enum Roles {
        LengthRole = Qt::UserRole + 1,
        HiddenRole,
        NameRole,
    };

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void setSourceModel(QAbstractItemModel *sourceModel);
    QAbstractItemModel *sourceModel() const;

    void setOrientation(Qt::Orientation orientation);
    Qt::Orientation orientation() const;

    int defaultLength() const;
    void setDefaultLength(int value);

    Q_INVOKABLE int length(int section) const;
    Q_INVOKABLE void setLength(int section, int len);
    Q_INVOKABLE bool isVisible(int section) const;
    Q_INVOKABLE void setVisible(int section, bool visible);
    Q_INVOKABLE int minimumLength(int section) const;

signals:
    void sourceModelChanged();
    void orientationChanged();
    void defaultLengthChanged();
    void sectionVisibilityChanged(int);

private slots:
    void onSourceItemsInserted(const QModelIndex &parent, int first, int last);
    void onSourceItemsRemoved(const QModelIndex &parent, int first, int last);
    void onSourceItemsMoved(
        const QModelIndex &sourceParent,
        int sourceStart,
        int sourceEnd,
        const QModelIndex &destinationParent,
        int destinationRow);
    void checkModelReset();

private:
    void setupModel();
    bool invalidSection(int section) const;

    struct Item
    {
        bool visible = true;
        int length;
    };

    QPointer<QAbstractItemModel> m_sourceModel;
    Qt::Orientation m_orientation = Qt::Horizontal;
    int m_defaultLength = 100;
    QList<Item> m_data;
};
