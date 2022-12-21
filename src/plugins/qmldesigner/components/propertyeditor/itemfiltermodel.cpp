// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemfiltermodel.h"

#include <abstractview.h>
#include <model.h>
#include <nodemetainfo.h>

#include <QFileDialog>
#include <QDirIterator>
#include <qmlmodelnodeproxy.h>

ItemFilterModel::ItemFilterModel(QObject *parent) :
    QObject(parent), m_typeFilter("QtQuick.Item"), m_lock(false), m_selectionOnly(false)
{
}

void ItemFilterModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{

    auto modelNodeBackendObject = modelNodeBackend.value<QObject*>();

    const auto backendObjectCasted =
            qobject_cast<const QmlDesigner::QmlModelNodeProxy *>(modelNodeBackendObject);

    if (backendObjectCasted)
        m_modelNode = backendObjectCasted->qmlObjectNode().modelNode();

    setupModel();
    emit modelNodeBackendChanged();
}

void ItemFilterModel::setTypeFilter(const QString &filter)
{
    if (m_typeFilter != filter) {
        m_typeFilter = filter;
        setupModel();
    }
}

void ItemFilterModel::setSelectionOnly(bool value)
{
    if (m_selectionOnly != value) {
        m_selectionOnly = value;
        setupModel();
    }
}

QString ItemFilterModel::typeFilter() const
{
    return m_typeFilter;
}

bool ItemFilterModel::selectionOnly() const
{
    return m_selectionOnly;
}

void ItemFilterModel::registerDeclarativeType()
{
    qmlRegisterType<ItemFilterModel>("HelperWidgets",2,0,"ItemFilterModel");
}

QVariant ItemFilterModel::modelNodeBackend() const
{
    return QVariant();
}


void ItemFilterModel::setupModel()
{
    if (!m_modelNode.isValid() || !m_modelNode.view()->isAttached())
        return;

    m_lock = true;
    m_model.clear();

    const auto nodes = m_selectionOnly ? m_modelNode.view()->selectedModelNodes()
                                       : m_modelNode.view()->allModelNodes();

    auto base = m_modelNode.model()->metaInfo(m_typeFilter.toUtf8());
    for (const QmlDesigner::ModelNode &node : nodes) {
        if (node.hasId() && node.metaInfo().isBasedOn(base))
            m_model.append(node.id());
    }

    m_lock = false;

    emit itemModelChanged();
}

QStringList ItemFilterModel::itemModel() const
{
    return m_model;
}
