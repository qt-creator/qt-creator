/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QDECLARATIVEDEBUGSERVICE_H
#define QDECLARATIVEDEBUGSERVICE_H

#include "../qmljsdebugger_global.h"
#include <qobject.h>

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeDebugServicePrivate;
class QMLJSDEBUGGER_EXTERN QDeclarativeDebugService : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDeclarativeDebugService)
    Q_DISABLE_COPY(QDeclarativeDebugService)

public:
    explicit QDeclarativeDebugService(const QString &, QObject *parent = 0);
    ~QDeclarativeDebugService();

    QString name() const;

    enum Status { NotConnected, Unavailable, Enabled };
    Status status() const;

    void sendMessage(const QByteArray &);

    static int idForObject(QObject *);
    static QObject *objectForId(int);

    static QString objectToString(QObject *obj);

    static bool isDebuggingEnabled();
    static bool hasDebuggingClient();

protected:
    virtual void statusChanged(Status);
    virtual void messageReceived(const QByteArray &);

private:
    friend class QDeclarativeDebugServer;
};

QT_END_NAMESPACE

#endif // QDECLARATIVEDEBUGSERVICE_H

