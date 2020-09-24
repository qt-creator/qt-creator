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

QDataStream &operator<<(QDataStream &out, const RequestModelNodePreviewImageCommand &command);
QDataStream &operator>>(QDataStream &in, RequestModelNodePreviewImageCommand &command);

QDebug operator <<(QDebug debug, const RequestModelNodePreviewImageCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::RequestModelNodePreviewImageCommand)
