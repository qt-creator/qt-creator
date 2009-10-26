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

#ifndef COMMUNICATIONSTARTER_H
#define COMMUNICATIONSTARTER_H

#include <QtCore/QSharedPointer>
#include <QtCore/QObject>

namespace trk {
class TrkDevice;
class BluetoothListener;
struct BaseCommunicationStarterPrivate;

/* BaseCommunicationStarter: A QObject that repeatedly tries to open a
 * trk device until a connection succeeds or a timeout occurs (emitting
 * signals), allowing to do something else in the foreground (local event loop
 * [say QMessageBox] or some asynchronous operation). If the initial
 * connection attempt in start() fails, the
 * virtual initializeStartupResources() is called to initialize resources
 * required to pull up the communication (namely Bluetooth listeners).
 * The base class can be used as is to prompt the user to launch TRK for a serial
 * communication as this requires no further resource setup. */

class BaseCommunicationStarter : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(BaseCommunicationStarter)
public:
    typedef QSharedPointer<TrkDevice> TrkDevicePtr;

    enum State { Running, Connected, TimedOut };

    explicit BaseCommunicationStarter(const TrkDevicePtr& trkDevice, QObject *parent = 0);
    virtual ~BaseCommunicationStarter();

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
    void message(const QString &);

private slots:
    void slotTimer();

protected:
    virtual bool initializeStartupResources(QString *errorMessage);

private:
    inline void stopTimer();

    BaseCommunicationStarterPrivate *d;
};

/* AbstractBluetoothStarter: Repeatedly tries to open a trk Bluetooth
 * device. Note that in case a Listener is already running mode, the
 * connection will succeed immediately.
 * initializeStartupResources() is implemented to fire up the listener.
 * Introduces a new virtual createListener() that derived classes must
 * implement as a factory function that creates and sets up the
 * listener (mode, message connection, etc). */

class AbstractBluetoothStarter : public BaseCommunicationStarter {
    Q_OBJECT
    Q_DISABLE_COPY(AbstractBluetoothStarter)
public:

protected:
    explicit AbstractBluetoothStarter(const TrkDevicePtr& trkDevice, QObject *parent = 0);

    // Implemented to fire up the listener.
    virtual bool initializeStartupResources(QString *errorMessage);
    // New virtual: Overwrite to create and parametrize the listener.
    virtual BluetoothListener *createListener() = 0;
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

#endif // COMMUNICATIONSTARTER_H
