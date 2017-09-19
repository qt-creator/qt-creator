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

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QApplication>
#include <QDir>

#include <connectionserver.h>
#include <filepathcaching.h>
#include <refactoringserver.h>
#include <refactoringclientproxy.h>
#include <symbolindexing.h>

using ClangBackEnd::FilePathCaching;
using ClangBackEnd::RefactoringClientProxy;
using ClangBackEnd::RefactoringServer;
using ClangBackEnd::RefactoringDatabaseInitializer;
using ClangBackEnd::ConnectionServer;
using ClangBackEnd::SymbolIndexing;

QString processArguments(QCoreApplication &application)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt Creator Clang Refactoring Backend"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("connection"), QStringLiteral("Connection"));

    parser.process(application);

    if (parser.positionalArguments().isEmpty())
        parser.showHelp(1);

    return parser.positionalArguments().first();
}

int main(int argc, char *argv[])
try {
    //QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false"));

    QCoreApplication::setOrganizationName(QStringLiteral("QtProject"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("qt-project.org"));
    QCoreApplication::setApplicationName(QStringLiteral("ClangRefactoringBackend"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCoreApplication application(argc, argv);

    const QString connection =  processArguments(application);

    Sqlite::Database database{Utils::PathString{QDir::tempPath() + "/symbol.db"}};
    RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    FilePathCaching filePathCache{database};
    SymbolIndexing symbolIndexing{database, filePathCache};
    RefactoringServer clangCodeModelServer{symbolIndexing, filePathCache};
    ConnectionServer<RefactoringServer, RefactoringClientProxy> connectionServer(connection);
    connectionServer.start();
    connectionServer.setServer(&clangCodeModelServer);


    return application.exec();
} catch (const Sqlite::Exception &exception) {
    exception.printWarning();
}


