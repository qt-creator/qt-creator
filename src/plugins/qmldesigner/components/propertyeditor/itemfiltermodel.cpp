/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
    if (!m_modelNode.isValid())
        return;

    m_lock = true;
    m_model.clear();

    const auto nodes = m_selectionOnly ? m_modelNode.view()->selectedModelNodes() : m_modelNode.view()->allModelNodes();

    for (const QmlDesigner::ModelNode &node : nodes) {
        if (node.hasId() && node.metaInfo().isValid() && node.metaInfo().isSubclassOf(m_typeFilter.toUtf8()))
            m_model.append(node.id());
    }

    m_lock = false;

    emit itemModelChanged();
}

QStringList ItemFilterModel::itemModel() const
{
    return m_model;
}
