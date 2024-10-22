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
    NodeDumper(const ModelNode &node);

    virtual ~NodeDumper() = default;

    virtual int priority() const = 0;
    virtual bool isExportable() const = 0;
    virtual QJsonObject json(Component& component) const = 0;

    const NodeMetaInfo &metaInfo() const { return m_metaInfo; }
    const QmlObjectNode& objectNode() const { return m_objectNode; }
    QVariant propertyValue(const PropertyName &name) const;
    QString uuid() const;

    Model *model() const { return m_model; }

protected:
    const ModelNode &m_node;

private:
    QmlObjectNode m_objectNode;
    NodeMetaInfo m_metaInfo;
    Model *m_model = nullptr;
};
}
