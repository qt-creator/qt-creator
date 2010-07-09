/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

namespace Qt4ProjectManager {
namespace Internal {

template <class SshConnection> MaemoSshThread<SshConnection>::MaemoSshThread(const Core::SshServerInfo &server)
    : m_server(server), m_stopRequested(false)
{
}

template <class SshConnection> MaemoSshThread<SshConnection>::~MaemoSshThread()
{
    stop();
    wait();
}

template <class SshConnection> void MaemoSshThread<SshConnection>::run()
{
    if (m_stopRequested)
        return;

    if (!runInternal())
        m_error = m_connection->error();
}

template<class SshConnection> void MaemoSshThread<SshConnection>::stop()
{
    m_mutex.lock();
    m_stopRequested = true;
    m_waitCond.wakeAll();
    const bool hasConnection = !m_connection.isNull();
    if (hasConnection)
        m_connection->quit();
    m_mutex.unlock();
}

template<class SshConnection> void MaemoSshThread<SshConnection>::waitForStop()
{
    m_mutex.lock();
    while (!stopRequested())
        m_waitCond.wait(&m_mutex);
    m_mutex.unlock();
}

template<class SshConnection> void MaemoSshThread<SshConnection>::createConnection()
{
    typename SshConnection::Ptr connection = SshConnection::create(m_server);
    m_mutex.lock();
    m_connection = connection;
    m_mutex.unlock();
}

MaemoSshRunner::MaemoSshRunner(const Core::SshServerInfo &server,
                               const QString &command)
    : MaemoSshThread<Core::InteractiveSshConnection>(server),
      m_command(command)
{
    m_prompt = server.uname == QLatin1String("root") ? "#" : "$";
}

bool MaemoSshRunner::runInternal()
{
    createConnection();
    connect(m_connection.data(), SIGNAL(remoteOutput(QByteArray)),
            this, SLOT(handleRemoteOutput(QByteArray)));
    initState();
    if (!m_connection->start())
        return false;
    if (stopRequested())
        return true;

    waitForStop();
    return !m_connection->hasError();
}

void MaemoSshRunner::initState()
{
    m_endMarkerCount = 0;
    m_promptEncountered = false;
    m_potentialEndMarkerPrefix.clear();
}

void MaemoSshRunner::handleRemoteOutput(const QByteArray &curOutput)
{
    const QByteArray output
        = filterTerminalControlChars(m_potentialEndMarkerPrefix + curOutput);

    // Wait for a prompt before sending the command.
    if (!m_promptEncountered) {
        if (output.indexOf(m_prompt) != -1) {
            m_promptEncountered = true;

            /*
             * We don't have access to the remote process management, so we
             * try to track the lifetime of the process by adding a second command
             * that prints a rare character. When it occurs for the second time (the
             * first one is the echo from the remote terminal), we assume the
             * process has finished. If anyone actually prints this special character
             * in their application, they are out of luck.
             */
            const QString finalCommand = m_command + QLatin1String(";echo ")
                + QString::fromUtf8(EndMarker) + QLatin1Char('\n');
            if (!m_connection->sendInput(finalCommand.toUtf8()))
                stop();
        }
        return;
    }

    /*
     * The output the user should see is everything after the first
     * and before the last occurrence of our marker string.
     */
    int firstCharToEmit;
    int charsToEmitCount;
    const int endMarkerPos = output.indexOf(EndMarker);
    if (endMarkerPos != -1) {
        if (m_endMarkerCount++ == 0) {
            firstCharToEmit = endMarkerPos + EndMarker.count() + 1;
            int endMarkerPos2
                    = output.indexOf(EndMarker, firstCharToEmit);
            if (endMarkerPos2 != -1) {
                ++ m_endMarkerCount;
                charsToEmitCount = endMarkerPos2 - firstCharToEmit;
            } else {
                charsToEmitCount = -1;
            }
        } else {
            firstCharToEmit = m_potentialEndMarkerPrefix.count();
            charsToEmitCount = endMarkerPos - firstCharToEmit;
        }
    } else {
        if (m_endMarkerCount == 0) {
            charsToEmitCount = 0;
        } else {
            firstCharToEmit = m_potentialEndMarkerPrefix.count();
            charsToEmitCount = -1;
        }
    }

    if (charsToEmitCount != 0) {
        emit remoteOutput(QString::fromUtf8(output.data() + firstCharToEmit,
                                            charsToEmitCount));
    }
    if (m_endMarkerCount == 2)
        stop();

    m_potentialEndMarkerPrefix = output.right(EndMarker.count());
}

QByteArray MaemoSshRunner::filterTerminalControlChars(const QByteArray &data)
{
    QByteArray filteredData;
    for (int i = 0; i < data.size(); ++i) {
        if (data.at(i) == '\b') {
            if (filteredData.isEmpty()) {
                qWarning("Failed to filter terminal control characters, "
                    "remote output may not appear.");
            } else {
                filteredData.remove(filteredData.count() - 1, 1);
            }
        } else {
            filteredData.append(data.at(i));
        }
    }

    return filteredData;
}

const QByteArray MaemoSshRunner::EndMarker(QString(QChar(0x13a0)).toUtf8());


MaemoSshDeployer::MaemoSshDeployer(const Core::SshServerInfo &server,
    const QList<Core::SftpTransferInfo> &deploySpecs)
    : MaemoSshThread<Core::SftpConnection>(server),
      m_deploySpecs(deploySpecs)
{
}

bool MaemoSshDeployer::runInternal()
{
    createConnection();
    if (!m_connection->start())
        return false;
    if (stopRequested())
        return true;

    connect(m_connection.data(), SIGNAL(fileCopied(QString)),
            this, SIGNAL(fileCopied(QString)));
    return m_connection->transferFiles(m_deploySpecs);
}

} // namespace Internal
} // namespace Qt4ProjectManager
