/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef BLUETOOTHLISTENER_H
#define BLUETOOTHLISTENER_H

#include "symbianutils_global.h"

#include <QtCore/QObject>
#include <QtCore/QProcess>

namespace trk {
struct BluetoothListenerPrivate;

/* BluetoothListener: Starts a helper process watching connections on a
 * Bluetooth device, Linux only:
 * The rfcomm command is used. It process can be started in the background
 * while connection attempts (TrkDevice::open()) are made in the foreground. */

class SYMBIANUTILS_EXPORT BluetoothListener : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(BluetoothListener)
public:
    // The Mode property must be set before calling start().
    enum Mode {
        Listen, /* Terminate after client closed (read: Trk app
                 * on the phone terminated or disconnected).*/
        Watch   // Keep running, watch for next connection from client
    };

    explicit BluetoothListener(QObject *parent = 0);
    virtual ~BluetoothListener();

    Mode mode() const;
    void setMode(Mode m);

    bool start(const QString &device, QString *errorMessage);

    // Print messages on the console.
    bool printConsoleMessages() const;
    void setPrintConsoleMessages(bool p);

signals:
    void terminated();
    void message(const QString &);

public slots:
    void emitMessage(const QString &m); // accessed by starter

private slots:
    void slotStdOutput();
    void slotStdError();
    void slotProcessFinished(int, QProcess::ExitStatus);
    void slotProcessError(QProcess::ProcessError error);

private:
    int terminateProcess();

    BluetoothListenerPrivate *d;
};

} // namespace trk

#endif // BLUETOOTHLISTENER_H
