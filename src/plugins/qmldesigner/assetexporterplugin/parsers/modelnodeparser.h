/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#pragma once

#include "qmlobjectnode.h"

#include <QJsonObject>
#include <QByteArrayList>

namespace QmlDesigner {
class Component;
class ModelNode;

class ModelNodeParser
{
public:
    ModelNodeParser(const QByteArrayList &lineage, const ModelNode &node);

    virtual ~ModelNodeParser() = default;

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
