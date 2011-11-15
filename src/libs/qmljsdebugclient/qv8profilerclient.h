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

#ifndef QV8PROFILERCLIENT_H
#define QV8PROFILERCLIENT_H

#include "qdeclarativedebugclient.h"
#include "qmlprofilereventtypes.h"
#include "qmljsdebugclient_global.h"

#include <QtCore/QStack>
#include <QtCore/QStringList>

namespace QmlJsDebugClient {

class QMLJSDEBUGCLIENT_EXPORT QV8ProfilerClient : public QDeclarativeDebugClient
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording NOTIFY recordingChanged)

public:
    enum Message {
        V8Entry,
        V8Complete,

        V8MaximumMessage
    };

    QV8ProfilerClient(QDeclarativeDebugConnection *client);
    ~QV8ProfilerClient();

    bool isEnabled() const;
    bool isRecording() const;

public slots:
    void setRecording(bool);
    void clearData();
    void sendRecordingStatus();

signals:
    void complete();
    void v8range(int depth, const QString &function, const QString &filename,
               int lineNumber, double totalTime, double selfTime);

    void recordingChanged(bool arg);

    void enabledChanged();
    void cleared();

protected:
    virtual void statusChanged(Status);
    virtual void messageReceived(const QByteArray &);

private:
    class QV8ProfilerClientPrivate *d;
};

} // namespace QmlJsDebugClient

#endif // QV8PROFILERCLIENT_H
