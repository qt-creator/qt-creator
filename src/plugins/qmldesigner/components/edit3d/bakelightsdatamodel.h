// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include "modelnode.h"
#include "qmldesignercorelib_global.h"

#include <QAbstractListModel>
#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class AbstractView;

class BakeLightsDataModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct BakeData {
        QString id;               // node id. Also used as BakedLightmap.key
        PropertyName aliasProp;   // property id for component exposed models/lights
        bool isModel = false;     // false means light
        bool enabled = false;
        bool inUse = false;
        bool isTitle = false;     // if true, indicates a title row in UI
        bool isUnexposed = false; // if true, indicates a component with unexposed models/lights
        int resolution = 1024;
        QString bakeMode;
    };

    BakeLightsDataModel(AbstractView *view);
    ~BakeLightsDataModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool reset();
    void apply();

    ModelNode view3dNode() const { return m_view3dNode; }

private:
    QString commonPrefix();

    QPointer<AbstractView> m_view;
    QList<BakeData> m_dataList;
    ModelNode m_view3dNode;
};

QDebug operator<<(QDebug debug, const BakeLightsDataModel::BakeData &data);

} // namespace QmlDesigner
