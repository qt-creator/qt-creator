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

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QSharedPointer>

namespace trk {
class TrkDevice;
struct BluetoothListenerPrivate;
struct AbstractBluetoothStarterPrivate;

/* BluetoothListener: Starts a helper process watching connections on a
 * Bluetooth device, Linux only:
 * The rfcomm command is used. It process can be started in the background
 * while connection attempts (TrkDevice::open()) are made in the foreground. */

class BluetoothListener : public QObject
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

/* AbstractBluetoothStarter: Repeatedly tries to open a trk device
 * until a connection succeeds, allowing to do something else in the
 * foreground (local event loop or asynchronous operation).
 * Note that in case a Listener is already running in watch mode, it might
 * also happen that connection succeeds immediately.
 * Implementations must provide a factory function that creates and sets up the
 * listener (mode, message connection, etc). */

class AbstractBluetoothStarter : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(AbstractBluetoothStarter)
public:
   typedef QSharedPointer<TrkDevice> TrkDevicePtr;

    enum State { Running, Connected, TimedOut };

    virtual ~AbstractBluetoothStarter();

    int intervalMS() const;
    void setIntervalMS(int i);

    int attempts() const;
    void setAttempts(int a);

    QString device() const;
    void setDevice(const QString &);

    State state() const;
    QString errorString() const;

    enum StartResult {
        Started,               // Starter is now running.
        ConnectionSucceeded,   /* Initial connection attempt succeeded,
                                * no need to keep running. */
        StartError             // Error occurred during start.
    };

    StartResult start();

signals:
    void connected();
    void timeout();

private slots:
    void slotTimer();

protected:
    explicit AbstractBluetoothStarter(const TrkDevicePtr& trkDevice, QObject *parent = 0);
    // Overwrite to create and parametrize the listener.
    virtual BluetoothListener *createListener() = 0;

private:
    inline void stopTimer();

    AbstractBluetoothStarterPrivate *d;
};

/* ConsoleBluetoothStarter: Convenience class for console processes. Creates a
 * listener in "Listen" mode with the messages redirected to standard output. */

class ConsoleBluetoothStarter : public AbstractBluetoothStarter {
    Q_OBJECT
    Q_DISABLE_COPY(ConsoleBluetoothStarter)
public:

    static bool startBluetooth(const TrkDevicePtr& trkDevice,
                               QObject *listenerParent,
                               const QString &device,
                               int attempts,                               
                               QString *errorMessage);

protected:
    virtual BluetoothListener *createListener();

private:
    explicit ConsoleBluetoothStarter(const TrkDevicePtr& trkDevice,
                                      QObject *listenerParent,
                                      QObject *parent = 0);

    QObject *m_listenerParent;
};

} // namespace trk

#endif // BLUETOOTHLISTENER_H
