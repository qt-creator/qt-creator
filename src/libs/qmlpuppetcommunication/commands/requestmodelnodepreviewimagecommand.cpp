// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "requestmodelnodepreviewimagecommand.h"

#include <QDataStream>
#include <QDebug>

namespace QmlDesigner {

RequestModelNodePreviewImageCommand::RequestModelNodePreviewImageCommand() = default;

RequestModelNodePreviewImageCommand::RequestModelNodePreviewImageCommand(qint32 id, const QSize &size,
                                                                         const QString &componentPath,
                                                                         qint32 renderItemId)
    : m_instanceId(id)
    , m_size(size)
    , m_componentPath(componentPath)
    , m_renderItemId(renderItemId)
{
}

qint32 RequestModelNodePreviewImageCommand::instanceId() const
{
    return m_instanceId;
}

QSize QmlDesigner::RequestModelNodePreviewImageCommand::size() const
{
    return m_size;
}

QString RequestModelNodePreviewImageCommand::componentPath() const
{
    return m_componentPath;
}

qint32 RequestModelNodePreviewImageCommand::renderItemId() const
{
    return m_renderItemId;
}

QDataStream &operator<<(QDataStream &out, const RequestModelNodePreviewImageCommand &command)
{
    out << int(command.instanceId());
    out << command.size();
    out << command.componentPath();
    out << command.renderItemId();

    return out;
}

QDataStream &operator>>(QDataStream &in, RequestModelNodePreviewImageCommand &command)
{
    in >> command.m_instanceId;
    in >> command.m_size;
    in >> command.m_componentPath;
    in >> command.m_renderItemId;
    return in;
}

QDebug operator <<(QDebug debug, const RequestModelNodePreviewImageCommand &command)
{
    return debug.nospace() << "RequestModelNodePreviewImageCommand("
                           << "instanceId: " << command.instanceId() << ", "
                           << "size: " << command.size() << ", "
                           << "componentPath: " << command.componentPath() << ", "
                           << "renderItemId: " << command.renderItemId() << ")";
}

} // namespace QmlDesigner
