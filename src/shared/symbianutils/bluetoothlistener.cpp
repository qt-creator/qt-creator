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

#include "bluetoothlistener.h"
#include "trkdevice.h"

#include <QtCore/QDebug>

#ifdef Q_OS_UNIX
#   include <unistd.h>
#   include <signal.h>
#else
#   include <windows.h>
#endif

// Process id helpers.
#ifdef Q_OS_WIN
inline DWORD processId(const QProcess &p)
{
    if (const Q_PID processInfoStruct = p.pid())
        return processInfoStruct->dwProcessId;
    return 0;
}
#else
inline Q_PID processId(const QProcess &p)
{
    return p.pid();
}
#endif


enum { debug = 0 };

namespace trk {

struct BluetoothListenerPrivate {
    BluetoothListenerPrivate();
    QString device;
    QProcess process;
#ifdef Q_OS_WIN
    DWORD pid;
#else
    Q_PID pid;
#endif
    bool printConsoleMessages;
    BluetoothListener::Mode mode;
};

BluetoothListenerPrivate::BluetoothListenerPrivate() :
    pid(0),
    printConsoleMessages(false),
    mode(BluetoothListener::Listen)
{
}

BluetoothListener::BluetoothListener(QObject *parent) :
    QObject(parent),
    d(new BluetoothListenerPrivate)
{
    d->process.setProcessChannelMode(QProcess::MergedChannels);

    connect(&d->process, SIGNAL(readyReadStandardError()),
            this, SLOT(slotStdError()));
    connect(&d->process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(slotStdOutput()));
    connect(&d->process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slotProcessFinished(int,QProcess::ExitStatus)));
    connect(&d->process, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(slotProcessError(QProcess::ProcessError)));
}

BluetoothListener::~BluetoothListener()
{
    const int trc = terminateProcess();
    if (debug)
        qDebug() << "~BluetoothListener: terminated" << trc;
    delete d;
}

BluetoothListener::Mode BluetoothListener::mode() const
{
    return d->mode;
}

void BluetoothListener::setMode(Mode m)
{
    d->mode = m;
}

bool BluetoothListener::printConsoleMessages() const
{
    return d->printConsoleMessages;
}

void BluetoothListener::setPrintConsoleMessages(bool p)
{
    d->printConsoleMessages = p;
}

int BluetoothListener::terminateProcess()
{
    enum { TimeOutMS = 200 };
    if (debug)
        qDebug() << "terminateProcess" << d->process.pid() << d->process.state();
    if (d->process.state() == QProcess::NotRunning)
        return -1;
    emitMessage(tr("%1: Stopping listener %2...").arg(d->device).arg(processId(d->process)));
    // When listening, the process should terminate by itself after closing the connection
    if (mode() == Listen && d->process.waitForFinished(TimeOutMS))
        return 0;
#ifdef Q_OS_UNIX
    kill(d->process.pid(), SIGHUP); // Listens for SIGHUP
    if (d->process.waitForFinished(TimeOutMS))
        return 1;
#endif
    d->process.terminate();
    if (d->process.waitForFinished(TimeOutMS))
        return 2;
    d->process.kill();
    return 3;
}

bool BluetoothListener::start(const QString &device, QString *errorMessage)
{
    if (d->process.state() != QProcess::NotRunning) {
        *errorMessage = QLatin1String("Internal error: Still running.");
        return false;
    }
    d->device = device;
    const QString binary = QLatin1String("rfcomm");
    QStringList arguments;
    arguments << QLatin1String("-r")
              << (d->mode == Listen ? QLatin1String("listen") : QLatin1String("watch"))
              << device << QString(QLatin1Char('1'));
    if (debug)
        qDebug() << binary << arguments;
    emitMessage(tr("%1: Starting Bluetooth listener %2...").arg(device, binary));
    d->pid = 0;
    d->process.start(binary, arguments);
    if (!d->process.waitForStarted()) {
        *errorMessage = tr("Unable to run '%1': %2").arg(binary, d->process.errorString());
        return false;
    }
    d->pid = processId(d->process); // Forgets it after crash/termination
    emitMessage(tr("%1: Bluetooth listener running (%2).").arg(device).arg(processId(d->process)));
    return true;
}

void BluetoothListener::slotStdOutput()
{
    emitMessage(QString::fromLocal8Bit(d->process.readAllStandardOutput()));
}

void BluetoothListener::emitMessage(const QString &m)
{
    if (d->printConsoleMessages || debug)
        qDebug("%s\n", qPrintable(m));
    emit message(m);
}

void BluetoothListener::slotStdError()
{
    emitMessage(QString::fromLocal8Bit(d->process.readAllStandardError()));
}

void BluetoothListener::slotProcessFinished(int ex, QProcess::ExitStatus state)
{
    switch (state) {
    case QProcess::NormalExit:
        emitMessage(tr("%1: Process %2 terminated with exit code %3.")
                    .arg(d->device).arg(d->pid).arg(ex));
        break;
    case QProcess::CrashExit:
        emitMessage(tr("%1: Process %2 crashed.").arg(d->device).arg(d->pid));
        break;
    }
    emit terminated();
}

void BluetoothListener::slotProcessError(QProcess::ProcessError error)
{
    emitMessage(tr("%1: Process error %2: %3")
        .arg(d->device).arg(error).arg(d->process.errorString()));
}

} // namespace trk
