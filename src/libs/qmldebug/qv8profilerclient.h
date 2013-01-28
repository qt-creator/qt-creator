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

#ifndef QV8PROFILERCLIENT_H
#define QV8PROFILERCLIENT_H

#include "qmldebugclient.h"
#include "qmlprofilereventtypes.h"
#include "qmldebug_global.h"

#include <QStack>
#include <QStringList>

namespace QmlDebug {

class QMLDEBUG_EXPORT QV8ProfilerClient : public QmlDebugClient
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording NOTIFY recordingChanged)

public:
    enum Message {
        V8Entry,
        V8Complete,
        V8SnapshotChunk,
        V8SnapshotComplete,
        V8ProfilingStarted,

        V8MaximumMessage
    };

    QV8ProfilerClient(QmlDebugConnection *client);
    ~QV8ProfilerClient();

    bool isEnabled() const;
    bool isRecording() const;
    void setRecording(bool);

public slots:
    void clearData();
    void sendRecordingStatus();

signals:
    void complete();
    void v8range(int depth, const QString &function, const QString &filename,
               int lineNumber, double totalTime, double selfTime);

    void recordingChanged(bool arg);

    void enabledChanged();
    void cleared();

private:
    void setRecordingFromServer(bool);

protected:
    virtual void statusChanged(ClientStatus);
    virtual void messageReceived(const QByteArray &);

private:
    class QV8ProfilerClientPrivate *d;
};

} // namespace QmlDebug

#endif // QV8PROFILERCLIENT_H
