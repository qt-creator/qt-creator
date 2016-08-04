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

#include "echoclangcodemodelserver.h"

#include <clangbackendipc/clangcodemodelclientproxy.h>
#include <clangbackendipc/connectionserver.h>

#include <QCoreApplication>

using ClangBackEnd::ClangCodeModelClientProxy;
using ClangBackEnd::ConnectionServer;
using ClangBackEnd::EchoClangCodeModelServer;

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setOrganizationDomain("qt-project.org");
    QCoreApplication::setApplicationName("EchoCodeModelBackend");
    QCoreApplication::setApplicationVersion("1.0.0");

    QCoreApplication application(argc, argv);

    if (application.arguments().count() != 2) {
        qWarning() << "wrong argument count";
        return 1;
    }

    EchoClangCodeModelServer echoClangCodeModelServer;
    ConnectionServer<EchoClangCodeModelServer, ClangCodeModelClientProxy> connectionServer(application.arguments()[1]);
    connectionServer.start();
    connectionServer.setServer(&echoClangCodeModelServer);

    return application.exec();
}
