// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmetatype.h>
#include <QString>
#include <QDataStream>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class InstanceContainer;

QDataStream &operator<<(QDataStream &out, const InstanceContainer &container);
QDataStream &operator>>(QDataStream &in, InstanceContainer &container);

class InstanceContainer
{
    friend QDataStream &operator>>(QDataStream &in, InstanceContainer &container);

public:
    enum NodeSourceType {
        NoSource = 0,
        CustomParserSource = 1,
        ComponentSource = 2
    };

    enum NodeMetaType {
        ObjectMetaType,
        ItemMetaType
    };

    enum NodeFlag {
        ParentTakesOverRendering = 1
    };

     Q_DECLARE_FLAGS(NodeFlags, NodeFlag)

    InstanceContainer();
    InstanceContainer(qint32 instanceId,
                      const TypeName &type,
                      int majorNumber,
                      int minorNumber,
                      const QString &componentPath,
                      const QString &nodeSource,
                      NodeSourceType nodeSourceType,
                      NodeMetaType metaType,
                      NodeFlags metaFlags);

    qint32 instanceId() const;
    TypeName type() const;
    int majorNumber() const;
    int minorNumber() const;
    QString componentPath() const;
    QString nodeSource() const;
    NodeSourceType nodeSourceType() const;
    NodeMetaType metaType() const;
    bool checkFlag(NodeFlag flag) const;
    NodeFlags metaFlags() const;

private:
    qint32 m_instanceId = -1;
    TypeName m_type;
    qint32 m_majorNumber = -1;
    qint32 m_minorNumber = -1;
    QString m_componentPath;
    QString m_nodeSource;
    qint32 m_nodeSourceType = 0;
    qint32 m_metaType = 0;
    qint32 m_metaFlags = 0;
};

QDebug operator <<(QDebug debug, const InstanceContainer &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InstanceContainer)
Q_DECLARE_OPERATORS_FOR_FLAGS(QmlDesigner::InstanceContainer::NodeFlags)
