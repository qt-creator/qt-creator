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

#include "qtsingleapplication.h"
#include "qtlocalpeer.h"

#include <QWidget>
#include <QFileOpenEvent>

namespace SharedTools {

void QtSingleApplication::sysInit(const QString &appId)
{
    actWin = 0;
    firstPeer = new QtLocalPeer(this, appId);
    connect(firstPeer, SIGNAL(messageReceived(QString)), SIGNAL(messageReceived(QString)));
    pidPeer = new QtLocalPeer(this, appId + QLatin1Char('-') + QString::number(QCoreApplication::applicationPid(), 10));
    connect(pidPeer, SIGNAL(messageReceived(QString)), SIGNAL(messageReceived(QString)));
}


QtSingleApplication::QtSingleApplication(int &argc, char **argv, bool GUIenabled)
    : QApplication(argc, argv, GUIenabled)
{
    sysInit();
}


QtSingleApplication::QtSingleApplication(const QString &appId, int &argc, char **argv)
    : QApplication(argc, argv)
{
    this->appId = appId;
    sysInit(appId);
}

bool QtSingleApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *foe = static_cast<QFileOpenEvent*>(event);
        emit fileOpenRequest(foe->file());
        return true;
    }
    return QApplication::event(event);
}

bool QtSingleApplication::isRunning(qint64 pid)
{
    if (pid == -1)
        return firstPeer->isClient();

    QtLocalPeer peer(this, appId + QLatin1Char('-') + QString::number(pid, 10));
    return peer.isClient();
}

void QtSingleApplication::initialize(bool)
{
    firstPeer->isClient();
    pidPeer->isClient();
}

bool QtSingleApplication::sendMessage(const QString &message, int timeout, qint64 pid)
{
    if (pid == -1)
        return firstPeer->sendMessage(message, timeout);

    QtLocalPeer peer(this, appId + QLatin1Char('-') + QString::number(pid, 10));
    return peer.sendMessage(message, timeout);
}

QString QtSingleApplication::id() const
{
    return firstPeer->applicationId();
}

QString QtSingleApplication::applicationId() const
{
    return appId;
}

void QtSingleApplication::setActivationWindow(QWidget *aw, bool activateOnMessage)
{
    actWin = aw;
    if (activateOnMessage) {
        connect(firstPeer, SIGNAL(messageReceived(QString)), this, SLOT(activateWindow()));
        connect(pidPeer, SIGNAL(messageReceived(QString)), this, SLOT(activateWindow()));
    } else {
        disconnect(firstPeer, SIGNAL(messageReceived(QString)), this, SLOT(activateWindow()));
        disconnect(pidPeer, SIGNAL(messageReceived(QString)), this, SLOT(activateWindow()));
    }
}


QWidget* QtSingleApplication::activationWindow() const
{
    return actWin;
}


void QtSingleApplication::activateWindow()
{
    if (actWin) {
        actWin->setWindowState(actWin->windowState() & ~Qt::WindowMinimized);
        actWin->raise();
        actWin->activateWindow();
    }
}

} // namespace SharedTools
