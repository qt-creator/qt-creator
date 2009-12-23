/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemosshthread.h"

#ifdef USE_SSH_LIB

namespace Qt4ProjectManager {
namespace Internal {

MaemoSshThread::MaemoSshThread(const MaemoDeviceConfig &devConf)
    : m_devConf(devConf)
{
}

MaemoSshThread::~MaemoSshThread()
{
    stop();
    wait();
}

void MaemoSshThread::run()
{
    try {
        runInternal();
    } catch (const MaemoSshException &e) {
        m_error = e.error();
    }
}

void MaemoSshThread::stop()
{
    m_connection->stop();
}

void MaemoSshThread::setConnection(const MaemoSshConnection::Ptr &connection)
{
    m_connection = connection;
    emit connectionEstablished();
}

MaemoSshRunner::MaemoSshRunner(const MaemoDeviceConfig &devConf,
                               const QString &command)
    : MaemoSshThread(devConf), m_command(command)
{
}

void MaemoSshRunner::runInternal()
{
    MaemoInteractiveSshConnection::Ptr connection
        = MaemoInteractiveSshConnection::create(m_devConf);
    setConnection(connection);
    connect(connection.data(), SIGNAL(remoteOutput(QString)),
            this, SIGNAL(remoteOutput(QString)));
    connection->runCommand(m_command);
}

MaemoSshDeployer::MaemoSshDeployer(const MaemoDeviceConfig &devConf,
    const QStringList &filePaths, const QStringList &targetDirs)
    : MaemoSshThread(devConf), m_filePaths(filePaths), m_targetDirs(targetDirs)
{
}

void MaemoSshDeployer::runInternal()
{
    MaemoSftpConnection::Ptr connection
        = MaemoSftpConnection::create(m_devConf);
    setConnection(connection);
    connect(connection.data(), SIGNAL(fileCopied(QString)),
            this, SIGNAL(fileCopied(QString)));
    connection->transferFiles(m_filePaths, m_targetDirs);
}

} // namespace Internal
} // namespace Qt4ProjectManager

#endif
