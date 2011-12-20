/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QDEBUGMESSAGECLIENT_H
#define QDEBUGMESSAGECLIENT_H

#include "qdeclarativedebugclient.h"
#include "qmljsdebugclient_global.h"

namespace QmlJsDebugClient {

class QDebugMessageClientPrivate;
class QMLJSDEBUGCLIENT_EXPORT QDebugMessageClient : public QDeclarativeDebugClient
{
    Q_OBJECT

public:
    explicit QDebugMessageClient(QDeclarativeDebugConnection *client);
    ~QDebugMessageClient();

protected:
    virtual void statusChanged(Status status);
    virtual void messageReceived(const QByteArray &);

signals:
    void newStatus(QDeclarativeDebugClient::Status);
    void message(QtMsgType, const QString &);

private:
    class QDebugMessageClientPrivate *d;
    Q_DISABLE_COPY(QDebugMessageClient)
};

} // namespace QmlJsDebugClient

#endif // QDEBUGMESSAGECLIENT_H
