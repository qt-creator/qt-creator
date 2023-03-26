// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "qmlobjectnode.h"

#include <QJsonObject>
#include <QByteArrayList>

namespace QmlDesigner {
class Component;
class ModelNode;

class NodeDumper
{
public:
    NodeDumper(const QByteArrayList &lineage, const ModelNode &node);

    virtual ~NodeDumper() = default;

    virtual int priority() const = 0;
    virtual bool isExportable() const = 0;
    virtual QJsonObject json(Component& component) const = 0;

    const QByteArrayList& lineage() const { return m_lineage; }
    const QmlObjectNode& objectNode() const { return m_objectNode; }
    QVariant propertyValue(const PropertyName &name) const;
    QString uuid() const;

protected:
    const ModelNode &m_node;

private:
    QmlObjectNode m_objectNode;
    QByteArrayList m_lineage;
};
}
