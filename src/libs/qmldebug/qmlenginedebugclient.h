/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef QMLENGINEDEBUGCLIENT_H
#define QMLENGINEDEBUGCLIENT_H

#include "baseenginedebugclient.h"

namespace QmlDebug {

class QmlDebugConnection;

class QMLDEBUG_EXPORT QmlEngineDebugClient : public BaseEngineDebugClient
{
    Q_OBJECT
public:
    explicit QmlEngineDebugClient(QmlDebugConnection *conn);

    quint32 setBindingForObject(int objectDebugId, const QString &propertyName,
                                const QVariant &bindingExpression,
                                bool isLiteralValue,
                                QString source, int line);
    quint32 resetBindingForObject(int objectDebugId, const QString &propertyName);
    quint32 setMethodBody(int objectDebugId, const QString &methodName,
                          const QString &methodBody);

protected:
    void messageReceived(const QByteArray &data);
};

} // namespace QmlDebug

#endif // QMLENGINEDEBUGCLIENT_H
