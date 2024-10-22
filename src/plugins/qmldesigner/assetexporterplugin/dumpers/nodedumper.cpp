// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "nodedumper.h"
#include "assetexportpluginconstants.h"

#include <auxiliarydataproperties.h>

namespace QmlDesigner {
NodeDumper::NodeDumper(const ModelNode &node)
    : m_node(node)
    , m_objectNode(node)
    , m_metaInfo(node.metaInfo())
    , m_model{node.model()}
{

}

QVariant NodeDumper::propertyValue(const PropertyName &name) const
{
    return m_objectNode.instanceValue(name);
}

QString NodeDumper::uuid() const
{
    return m_node.auxiliaryDataWithDefault(uuidProperty).toString();
}

}
