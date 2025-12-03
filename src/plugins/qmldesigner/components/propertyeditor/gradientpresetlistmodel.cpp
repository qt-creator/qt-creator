// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradientpresetlistmodel.h"
#include "gradientpresetitem.h"
#include "propertyeditortracing.h"

#include <QHash>
#include <QByteArray>
#include <QDebug>

using QmlDesigner::PropertyEditorTracing::category;

GradientPresetListModel::GradientPresetListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    NanotraceHR::Tracer tracer{"gradient preset list model constructor", category()};

    m_roleNames = {{static_cast<int>(GradientPresetItem::Property::objectNameRole), "objectName"},
                   {static_cast<int>(GradientPresetItem::Property::stopsPosListRole), "stopsPosList"},
                   {static_cast<int>(GradientPresetItem::Property::stopsColorListRole),
                    "stopsColorList"},
                   {static_cast<int>(GradientPresetItem::Property::stopListSizeRole), "stopListSize"},
                   {static_cast<int>(GradientPresetItem::Property::presetNameRole), "presetName"},
                   {static_cast<int>(GradientPresetItem::Property::presetIDRole), "presetID"}};
}

GradientPresetListModel::~GradientPresetListModel()
{
    NanotraceHR::Tracer tracer{"gradient preset list model destructor", category()};

    clearItems();
}

int GradientPresetListModel::rowCount(const QModelIndex & /*parent*/) const
{
    NanotraceHR::Tracer tracer{"gradient preset list model row count", category()};

    return m_items.size();
}

QVariant GradientPresetListModel::data(const QModelIndex &index, int role) const
{
    NanotraceHR::Tracer tracer{"gradient preset list model data", category()};

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
    NanotraceHR::Tracer tracer{"gradient preset list model role names", category()};

    return m_roleNames;
}

void GradientPresetListModel::clearItems()
{
    NanotraceHR::Tracer tracer{"gradient preset list model clear items", category()};

    beginResetModel();
    m_items.clear();
    endResetModel();
}

void GradientPresetListModel::addItem(const GradientPresetItem &element)
{
    NanotraceHR::Tracer tracer{"gradient preset list model add item", category()};

    beginResetModel();
    m_items.append(element);
    endResetModel();
}

const QList<GradientPresetItem> &GradientPresetListModel::items() const
{
    NanotraceHR::Tracer tracer{"gradient preset list model items", category()};

    return m_items;
}

void GradientPresetListModel::sortItems()
{
    NanotraceHR::Tracer tracer{"gradient preset list model sort items", category()};

    std::ranges::sort(m_items, {}, &GradientPresetItem::presetID);
}

void GradientPresetListModel::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"gradient preset list model register declarative type", category()};

    qmlRegisterType<GradientPresetListModel>("HelperWidgets", 2, 0, "GradientPresetListModel");
}
