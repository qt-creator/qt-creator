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

#include "../qtcreatorcrashhandler/crashhandlersetup.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLoggingCategory>

#include <connectionserver.h>
#include <clangcodemodelserver.h>
#include <clangcodemodelclientproxy.h>

using ClangBackEnd::ClangCodeModelClientProxy;
using ClangBackEnd::ClangCodeModelServer;
using ClangBackEnd::ConnectionServer;

QString processArguments(QCoreApplication &application)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt Creator Clang backend process."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("connection"), QStringLiteral("Connection"));

    parser.process(application);

    if (parser.positionalArguments().isEmpty())
        parser.showHelp(1);

    return parser.positionalArguments().first();
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false"));

    QCoreApplication::setOrganizationName(QStringLiteral("QtProject"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("qt-project.org"));
    QCoreApplication::setApplicationName(QStringLiteral("ClangBackend"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QCoreApplication application(argc, argv);
    CrashHandlerSetup setupCrashHandler(QCoreApplication::applicationName(),
                                        CrashHandlerSetup::DisableRestart);

    const QString connection = processArguments(application);

    clang_toggleCrashRecovery(true);
    clang_enableStackTraces();

    ClangCodeModelServer clangCodeModelServer;
    ConnectionServer<ClangCodeModelServer, ClangCodeModelClientProxy> connectionServer(connection);
    connectionServer.start();
    connectionServer.setServer(&clangCodeModelServer);

    return application.exec();
}


