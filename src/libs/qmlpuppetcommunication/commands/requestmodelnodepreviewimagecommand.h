// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/qmetatype.h>
#include <QtCore/qdatastream.h>
#include <QtGui/qevent.h>

#include "instancecontainer.h"

namespace QmlDesigner {

class RequestModelNodePreviewImageCommand
{
    friend QDataStream &operator>>(QDataStream &in, RequestModelNodePreviewImageCommand &command);
    friend QDebug operator <<(QDebug debug, const RequestModelNodePreviewImageCommand &command);

public:
    RequestModelNodePreviewImageCommand();
    explicit RequestModelNodePreviewImageCommand(qint32 id, const QSize &size,
                                                 const QString &componentPath, qint32 renderItemId);

    qint32 instanceId() const;
    QSize size() const;
    QString componentPath() const;
    qint32 renderItemId() const;

private:
    qint32 m_instanceId;
    QSize m_size;
    QString m_componentPath;
    qint32 m_renderItemId;
};

inline bool operator==(const RequestModelNodePreviewImageCommand &first,
                       const RequestModelNodePreviewImageCommand &second)
{
    return first.instanceId() == second.instanceId()
        && first.size() == second.size()
        && first.componentPath() == second.componentPath()
        && first.renderItemId() == second.renderItemId();
}

inline size_t qHash(const RequestModelNodePreviewImageCommand &key, size_t seed = 0)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return ::qHash(key.instanceId(), seed)
            ^ ::qHash(std::make_pair(key.size().width(), key.size().height()), seed)
            ^ ::qHash(key.componentPath(), seed) ^ ::qHash(key.renderItemId(), seed);
#else
    return qHashMulti(seed, key.instanceId(), key.size(), key.componentPath(), key.renderItemId());
#endif
}

QDataStream &operator<<(QDataStream &out, const RequestModelNodePreviewImageCommand &command);
QDataStream &operator>>(QDataStream &in, RequestModelNodePreviewImageCommand &command);

QDebug operator <<(QDebug debug, const RequestModelNodePreviewImageCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::RequestModelNodePreviewImageCommand)
