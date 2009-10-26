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

#include "communicationstarter.h"
#include "bluetoothlistener.h"
#include "trkdevice.h"

#include <QtCore/QTimer>
#include <QtCore/QEventLoop>

namespace trk {
// --------------- AbstractBluetoothStarter
struct AbstractBluetoothStarterPrivate {
    explicit AbstractBluetoothStarterPrivate(const AbstractBluetoothStarter::TrkDevicePtr &d);

    const AbstractBluetoothStarter::TrkDevicePtr trkDevice;
    BluetoothListener *listener;
    QTimer *timer;
    int intervalMS;
    int attempts;
    int n;
    QString device;
    QString errorString;
    AbstractBluetoothStarter::State state;
};

AbstractBluetoothStarterPrivate::AbstractBluetoothStarterPrivate(const AbstractBluetoothStarter::TrkDevicePtr &d) :

        trkDevice(d),
        listener(0),
        timer(0),
        intervalMS(1000),
        attempts(-1),
        n(0),
        device(QLatin1String("/dev/rfcomm0")),
        state(AbstractBluetoothStarter::TimedOut)
{
}

AbstractBluetoothStarter::AbstractBluetoothStarter(const TrkDevicePtr &trkDevice, QObject *parent) :
        QObject(parent),
        d(new AbstractBluetoothStarterPrivate(trkDevice))
{
}

AbstractBluetoothStarter::~AbstractBluetoothStarter()
{
    stopTimer();
    delete d;
}

void AbstractBluetoothStarter::stopTimer()
{
    if (d->timer && d->timer->isActive())
        d->timer->stop();
}

AbstractBluetoothStarter::StartResult AbstractBluetoothStarter::start()
{
    if (state() == Running) {
        d->errorString = QLatin1String("Internal error, attempt to re-start AbstractBluetoothStarter.\n");
        return StartError;
    }
    // Before we instantiate timers, and such, try to open the device,
    // which should succeed if another listener is already running in
    // 'Watch' mode
    if (d->trkDevice->open(d->device , &(d->errorString)))
        return ConnectionSucceeded;
    // Fire up the listener
    d->n = 0;
    d->listener = createListener();
    if (!d->listener->start(d->device, &(d->errorString)))
        return StartError;
    // Start timer
    if (!d->timer) {
        d->timer = new QTimer;
        connect(d->timer, SIGNAL(timeout()), this, SLOT(slotTimer()));
    }
    d->timer->setInterval(d->intervalMS);
    d->timer->setSingleShot(false);
    d->timer->start();
    d->state = Running;
    return Started;
}

AbstractBluetoothStarter::State AbstractBluetoothStarter::state() const
{
    return d->state;
}

int AbstractBluetoothStarter::intervalMS() const
{
    return d->intervalMS;
}

void AbstractBluetoothStarter::setIntervalMS(int i)
{
    d->intervalMS = i;
    if (d->timer)
        d->timer->setInterval(i);
}

int AbstractBluetoothStarter::attempts() const
{
    return d->attempts;
}

void AbstractBluetoothStarter::setAttempts(int a)
{
    d->attempts = a;
}

QString AbstractBluetoothStarter::device() const
{
    return d->device;
}

void AbstractBluetoothStarter::setDevice(const QString &dv)
{
    d->device = dv;
}

QString AbstractBluetoothStarter::errorString() const
{
    return d->errorString;
}

void AbstractBluetoothStarter::slotTimer()
{
    ++d->n;
    // Check for timeout
    if (d->attempts >= 0 && d->n >= d->attempts) {
        stopTimer();
        d->errorString = tr("%1: timed out after %n attempts using an interval of %2ms.", 0, d->n)
                         .arg(d->device).arg(d->intervalMS);
        d->state = TimedOut;
        emit timeout();
    } else {
        // Attempt n to connect?
        if (d->trkDevice->open(d->device , &(d->errorString))) {
            stopTimer();
            const QString msg = tr("%1: Connection attempt %2 succeeded.").arg(d->device).arg(d->n);
            d->listener->emitMessage(msg);
            d->state = Connected;
            emit connected();
        } else {
            const QString msg = tr("%1: Connection attempt %2 failed: %3 (retrying)...")
                                .arg(d->device).arg(d->n).arg(d->errorString);
            d->listener->emitMessage(msg);
        }
    }
}

// -------- ConsoleBluetoothStarter
ConsoleBluetoothStarter::ConsoleBluetoothStarter(const TrkDevicePtr &trkDevice,
                                                 QObject *listenerParent,
                                                 QObject *parent) :
AbstractBluetoothStarter(trkDevice, parent),
m_listenerParent(listenerParent)
{
}

BluetoothListener *ConsoleBluetoothStarter::createListener()
{
    BluetoothListener *rc = new BluetoothListener(m_listenerParent);
    rc->setMode(BluetoothListener::Listen);
    rc->setPrintConsoleMessages(true);
    return rc;
}

bool ConsoleBluetoothStarter::startBluetooth(const TrkDevicePtr &trkDevice,
                                             QObject *listenerParent,
                                             const QString &device,
                                             int attempts,
                                             QString *errorMessage)
{
    // Set up a console starter to print to stdout.
    ConsoleBluetoothStarter starter(trkDevice, listenerParent);
    starter.setDevice(device);
    starter.setAttempts(attempts);
    switch (starter.start()) {
    case Started:
        break;
    case ConnectionSucceeded:
        return true;
    case StartError:
        *errorMessage = starter.errorString();
        return false;
    }
    // Run the starter with an event loop. @ToDo: Implement
    // some asynchronous keypress read to cancel.
    QEventLoop eventLoop;
    connect(&starter, SIGNAL(connected()), &eventLoop, SLOT(quit()));
    connect(&starter, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
    if (starter.state() != AbstractBluetoothStarter::Connected) {
        *errorMessage = starter.errorString();
        return false;
    }
    return true;
}
} // namespace trk
