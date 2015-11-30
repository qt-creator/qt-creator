/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qdebugmessageclient.h"

#include <QDataStream>

namespace QmlDebug {

QDebugMessageClient::QDebugMessageClient(QmlDebugConnection *client)
    : QmlDebugClient(QLatin1String("DebugMessages"), client)
{
}

QDebugMessageClient::~QDebugMessageClient()
{
}

void QDebugMessageClient::stateChanged(State state)
{
    emit newState(state);
}

void QDebugMessageClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    QByteArray command;
    ds >> command;

    if (command == "MESSAGE") {
        int type;
        int line;
        QByteArray debugMessage;
        QByteArray file;
        QByteArray function;
        ds >> type >> debugMessage >> file >> line >> function;
        QDebugContextInfo info;
        info.line = line;
        info.file = QString::fromUtf8(file);
        info.function = QString::fromUtf8(function);
        info.timestamp = -1;
        if (!ds.atEnd()) {
            QByteArray category;
            ds >> category;
            info.category = QString::fromUtf8(category);
            if (!ds.atEnd())
                ds >> info.timestamp;
        }
        emit message(QtMsgType(type), QString::fromUtf8(debugMessage), info);
    }
}

} // namespace QmlDebug
