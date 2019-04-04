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

#include "gradientpresetlistmodel.h"
#include "gradientpresetitem.h"

#include <QHash>
#include <QByteArray>
#include <QDebug>
#include <QSettings>

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
    return m_items.count();
}

QVariant GradientPresetListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && (index.row() >= 0) && (index.row() < m_items.count())) {
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
