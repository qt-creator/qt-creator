/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "pchmanagerconnectionclient.h"

#include <coreplugin/icore.h>

#include <utils/temporarydirectory.h>

#include <QCoreApplication>

namespace ClangPchManager {

namespace {

QString currentProcessId()
{
    return QString::number(QCoreApplication::applicationPid());
}

}

ClangPchManager::PchManagerConnectionClient::PchManagerConnectionClient(
        ClangBackEnd::PchManagerClientInterface *client)
    : ConnectionClient(Utils::TemporaryDirectory::masterDirectoryPath()
                       + QStringLiteral("/ClangPchManagerBackEnd-")
                       + currentProcessId()),
      m_serverProxy(client, ioDevice())
{
    m_processCreator.setTemporaryDirectoryPattern("clangpchmanagerbackend-XXXXXX");

    QDir pchsDirectory(Core::ICore::cacheResourcePath());
    pchsDirectory.mkdir("pchs");
    pchsDirectory.cd("pchs");
    m_processCreator.setArguments({connectionName(),
                                   Core::ICore::cacheResourcePath() + "/symbol-experimental-v1.db",
                                   pchsDirectory.absolutePath()});

    stdErrPrefixer().setPrefix("PchManagerConnectionClient.stderr: ");
    stdOutPrefixer().setPrefix("PchManagerConnectionClient.stdout: ");
}

PchManagerConnectionClient::~PchManagerConnectionClient()
{
    finishProcess();
}

ClangBackEnd::PchManagerServerProxy &ClangPchManager::PchManagerConnectionClient::serverProxy()
{
    return m_serverProxy;
}

void ClangPchManager::PchManagerConnectionClient::sendEndCommand()
{
    m_serverProxy.end();
}

void PchManagerConnectionClient::resetState()
{
    m_serverProxy.resetState();
}

QString PchManagerConnectionClient::outputName() const
{
    return QStringLiteral("PchManagerConnectionClient");
}

void PchManagerConnectionClient::newConnectedServer(QLocalSocket *localSocket)
{
    m_serverProxy.setLocalSocket(localSocket);
}

} // namespace ClangPchManager
