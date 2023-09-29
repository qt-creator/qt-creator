// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradientpresetlistmodel.h"
#include "gradientpresetitem.h"

#include <QHash>
#include <QByteArray>
#include <QDebug>

GradientPresetListModel::GradientPresetListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_roleNames
        = {{static_cast<int>(GradientPresetItem::Property::objectNameRole), "objectName"},
           {static_cast<int>(GradientPresetItem::Property::stopsPosListRole), "stopsPosList"},
           {static_cast<int>(GradientPresetItem::Property::stopsColorListRole), "stopsColorList"},
           {static_cast<int>(GradientPresetItem::Property::stopListSizeRole), "stopListSize"},
           {static_cast<int>(GradientPresetItem::Property::presetNameRole), "presetName"},
           {static_cast<int>(GradientPresetItem::Property::presetIDRole), "presetID"}};
}

GradientPresetListModel::~GradientPresetListModel()
{
    clearItems();
}

int GradientPresetListModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_items.size();
}

QVariant GradientPresetListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && (index.row() >= 0) && (index.row() < m_items.size())) {
        if (m_roleNames.contains(role)) {
            QVariant value = m_items.at(index.row())
                                 .getProperty(static_cast<GradientPresetItem::Property>(role));

            if (auto model = qobject_cast<GradientPresetListModel *>(value.value<QObject *>()))
                return QVariant::fromValue(model);

            return value;
        }

        qWarning() << Q_FUNC_INFO << "invalid role requested";
        return QVariant();
    }

    qWarning() << Q_FUNC_INFO << "invalid index requested";
    return QVariant();
}

QHash<int, QByteArray> GradientPresetListModel::roleNames() const
{
    return m_roleNames;
}

void GradientPresetListModel::clearItems()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

void GradientPresetListModel::addItem(const GradientPresetItem &element)
{
    beginResetModel();
    m_items.append(element);
    endResetModel();
}

const QList<GradientPresetItem> &GradientPresetListModel::items() const
{
    return m_items;
}

void GradientPresetListModel::sortItems()
{
    auto itemSort = [](const GradientPresetItem &first, const GradientPresetItem &second) {
        return (static_cast<int>(first.presetID()) < static_cast<int>(second.presetID()));
    };

    std::sort(m_items.begin(), m_items.end(), itemSort);
}

void GradientPresetListModel::registerDeclarativeType()
{
    qmlRegisterType<GradientPresetListModel>("HelperWidgets", 2, 0, "GradientPresetListModel");
}
