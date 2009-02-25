/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "qtsinglecoreapplication.h"
#include "qtlocalpeer.h"

namespace SharedTools {

QtSingleCoreApplication::QtSingleCoreApplication(int &argc, char **argv)
    : QCoreApplication(argc, argv)
{
    peer = new QtLocalPeer(this);
    connect(peer, SIGNAL(messageReceived(QString)), SIGNAL(messageReceived(QString)));
}


QtSingleCoreApplication::QtSingleCoreApplication(const QString &appId, int &argc, char **argv)
    : QCoreApplication(argc, argv)
{
    peer = new QtLocalPeer(this, appId);
    connect(peer, SIGNAL(messageReceived(QString)), SIGNAL(messageReceived(QString)));
}


bool QtSingleCoreApplication::isRunning()
{
    return peer->isClient();
}


bool QtSingleCoreApplication::sendMessage(const QString &message, int timeout)
{
    return peer->sendMessage(message, timeout);
}


QString QtSingleCoreApplication::id() const
{
    return peer->applicationId();
}

} // namespace SharedTools
