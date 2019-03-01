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

#include "clangcodemodelconnectionclient.h"

#include <utils/environment.h>
#include <utils/temporarydirectory.h>

#include <QCoreApplication>

namespace ClangBackEnd {

namespace {

QString currentProcessId()
{
    return QString::number(QCoreApplication::applicationPid());
}

}

ClangCodeModelConnectionClient::ClangCodeModelConnectionClient(ClangCodeModelClientInterface *client)
    : ConnectionClient(Utils::TemporaryDirectory::masterDirectoryPath()
                       + QStringLiteral("/ClangBackEnd-") + currentProcessId())
    , m_serverProxy(client)
{
    m_processCreator.setTemporaryDirectoryPattern("clangbackend-XXXXXX");
    m_processCreator.setArguments({connectionName()});

    Utils::Environment environment;
    environment.set(QStringLiteral("LIBCLANG_NOTHREADS"), QString());
    environment.set(QStringLiteral("LIBCLANG_DISABLE_CRASH_RECOVERY"), QString());
    m_processCreator.setEnvironment(environment);

    stdErrPrefixer().setPrefix("clangbackend.stderr: ");
    stdOutPrefixer().setPrefix("clangbackend.stdout: ");
}

ClangCodeModelConnectionClient::~ClangCodeModelConnectionClient()
{
    finishProcess();
}

ClangCodeModelServerProxy &ClangCodeModelConnectionClient::serverProxy()
{
    return m_serverProxy;
}

void ClangCodeModelConnectionClient::sendEndCommand()
{
    m_serverProxy.end();
}

void ClangCodeModelConnectionClient::resetState()
{
    m_serverProxy.resetState();
}

QString ClangCodeModelConnectionClient::outputName() const
{
    return QStringLiteral("ClangCodeModelConnectionClient");
}

void ClangCodeModelConnectionClient::newConnectedServer(QLocalSocket *localSocket)
{
    m_serverProxy.setLocalSocket(localSocket);
}

} // namespace ClangBackEnd
