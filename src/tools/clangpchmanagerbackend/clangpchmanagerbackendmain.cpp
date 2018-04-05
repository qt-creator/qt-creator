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

#include <clangpathwatcher.h>
#include <connectionserver.h>
#include <environment.h>
#include <pchcreator.h>
#include <pchgenerator.h>
#include <pchmanagerserver.h>
#include <pchmanagerclientproxy.h>
#include <projectparts.h>
#include <filepathcaching.h>
#include <refactoringdatabaseinitializer.h>
#include <sqlitedatabase.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QLoggingCategory>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

using ClangBackEnd::ClangPathWatcher;
using ClangBackEnd::ConnectionServer;
using ClangBackEnd::PchCreator;
using ClangBackEnd::PchGenerator;
using ClangBackEnd::PchManagerClientProxy;
using ClangBackEnd::PchManagerServer;
using ClangBackEnd::ProjectParts;
using ClangBackEnd::FilePathCache;

class PchManagerApplication : public QCoreApplication
{
public:
    using QCoreApplication::QCoreApplication;

    bool notify(QObject *object, QEvent *event) override
    {
        try {
            return QCoreApplication::notify(object, event);
        } catch (Sqlite::Exception &exception) {
            exception.printWarning();
        }

        return false;
    }
};

class ApplicationEnvironment : public ClangBackEnd::Environment
{
public:
    ApplicationEnvironment(const QString &pchsPath)
        : m_pchBuildDirectoryPath(pchsPath)
    {
    }

    QString pchBuildDirectory() const override
    {
        return m_pchBuildDirectoryPath;
    }

    QString clangCompilerPath() const override
    {
        return QString(CLANG_COMPILER_PATH);
    }

    uint hardwareConcurrency() const override
    {
        return std::thread::hardware_concurrency();
    }

private:
    QString m_pchBuildDirectoryPath;
};

QStringList processArguments(QCoreApplication &application)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt Creator Clang PchManager Backend"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("connection"), QStringLiteral("Connection"));
    parser.addPositionalArgument(QStringLiteral("databasepath"), QStringLiteral("Database path"));
    parser.addPositionalArgument(QStringLiteral("pchspath"), QStringLiteral("PCHs path"));

    parser.process(application);

    if (parser.positionalArguments().isEmpty())
        parser.showHelp(1);

    return parser.positionalArguments();
}

int main(int argc, char *argv[])
{
    try {
        //QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false"));

        QCoreApplication::setOrganizationName(QStringLiteral("QtProject"));
        QCoreApplication::setOrganizationDomain(QStringLiteral("qt-project.org"));
        QCoreApplication::setApplicationName(QStringLiteral("ClangPchManagerBackend"));
        QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

        PchManagerApplication application(argc, argv);

        const QStringList arguments = processArguments(application);
        const QString connectionName = arguments[0];
        const QString databasePath = arguments[1];
        const QString pchsPath = arguments[2];

        Sqlite::Database database{Utils::PathString{databasePath}, 100000ms};
        ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
        ClangBackEnd::FilePathCaching filePathCache{database};
        ClangPathWatcher<QFileSystemWatcher, QTimer> includeWatcher(filePathCache);
        ApplicationEnvironment environment{pchsPath};
        PchGenerator<QProcess> pchGenerator(environment);
        PchCreator pchCreator(environment, filePathCache);
        pchCreator.setGenerator(&pchGenerator);
        ProjectParts projectParts;
        PchManagerServer clangPchManagerServer(includeWatcher,
                                               pchCreator,
                                               projectParts);
        includeWatcher.setNotifier(&clangPchManagerServer);
        pchGenerator.setNotifier(&clangPchManagerServer);

        ConnectionServer<PchManagerServer, PchManagerClientProxy> connectionServer;
        connectionServer.setServer(&clangPchManagerServer);
        connectionServer.start(connectionName);

        return application.exec();
    } catch (const Sqlite::Exception &exception) {
        exception.printWarning();
    }
}


