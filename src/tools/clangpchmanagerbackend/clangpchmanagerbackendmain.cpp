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

#include <builddependenciesprovider.h>
#include <builddependenciesstorage.h>
#include <builddependencycollector.h>
#include <clangpathwatcher.h>
#include <connectionserver.h>
#include <environment.h>
#include <executeinloop.h>
#include <filepathcaching.h>
#include <filesystem.h>
#include <generatedfiles.h>
#include <modifiedtimechecker.h>
#include <pchcreator.h>
#include <pchmanagerclientproxy.h>
#include <pchmanagerserver.h>
#include <pchtaskgenerator.h>
#include <pchtaskqueue.h>
#include <pchtasksmerger.h>
#include <precompiledheaderstorage.h>
#include <processormanager.h>
#include <progresscounter.h>
#include <projectpartsmanager.h>
#include <projectpartsstorage.h>
#include <refactoringdatabaseinitializer.h>
#include <sqlitedatabase.h>
#include <taskscheduler.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QLoggingCategory>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

using ClangBackEnd::ClangPathWatcher;
using ClangBackEnd::ConnectionServer;
using ClangBackEnd::FilePathCache;
using ClangBackEnd::FilePathView;
using ClangBackEnd::GeneratedFiles;
using ClangBackEnd::PchCreator;
using ClangBackEnd::PchManagerClientProxy;
using ClangBackEnd::PchManagerServer;
using ClangBackEnd::PrecompiledHeaderStorage;
using ClangBackEnd::ProjectPartsManager;
using ClangBackEnd::ProjectPartsStorage;
using ClangBackEnd::TimeStamp;

class PchManagerApplication final : public QCoreApplication
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

class ApplicationEnvironment final : public ClangBackEnd::Environment
{
public:
    ApplicationEnvironment(const QString &pchsPath, const QString &preIncludeSearchPath)
        : m_pchBuildDirectoryPath(pchsPath.toStdString())
        , m_preIncludeSearchPath(ClangBackEnd::FilePath{preIncludeSearchPath})
    {
    }

    Utils::PathString pchBuildDirectory() const override { return m_pchBuildDirectoryPath; }
    uint hardwareConcurrency() const override { return std::thread::hardware_concurrency(); }
    ClangBackEnd::NativeFilePathView preIncludeSearchPath() const override
    {
        return m_preIncludeSearchPath;
    }

private:
    Utils::PathString m_pchBuildDirectoryPath;
    ClangBackEnd::NativeFilePath m_preIncludeSearchPath;
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
    parser.addPositionalArgument(QStringLiteral("resourcepath"), QStringLiteral("Resource path"));

    parser.process(application);

    if (parser.positionalArguments().isEmpty())
        parser.showHelp(1);

    return parser.positionalArguments();
}

class PchCreatorManager final : public ClangBackEnd::ProcessorManager<ClangBackEnd::PchCreator>
{
public:
    using Processor = ClangBackEnd::PchCreator;
    PchCreatorManager(const ClangBackEnd::GeneratedFiles &generatedFiles,
                      ClangBackEnd::Environment &environment,
                      ClangBackEnd::FilePathCaching &filePathCache,
                      PchManagerServer &pchManagerServer,
                      ClangBackEnd::ClangPathWatcherInterface &pathWatcher,
                      ClangBackEnd::BuildDependenciesStorageInterface &buildDependenciesStorage)
        : ProcessorManager(generatedFiles)
        , m_environment(environment)
        , m_filePathCache(filePathCache)
        , m_pchManagerServer(pchManagerServer)
        , m_pathWatcher(pathWatcher)
        , m_buildDependenciesStorage(buildDependenciesStorage)
    {}

protected:
    std::unique_ptr<ClangBackEnd::PchCreator> createProcessor() const override
    {
        return std::make_unique<PchCreator>(m_environment,
                                            m_filePathCache,
                                            *m_pchManagerServer.client(),
                                            m_pathWatcher,
                                            m_buildDependenciesStorage);
    }

private:
    ClangBackEnd::Environment &m_environment;
    ClangBackEnd::FilePathCaching &m_filePathCache;
    ClangBackEnd::PchManagerServer &m_pchManagerServer;
    ClangBackEnd::ClangPathWatcherInterface &m_pathWatcher;
    ClangBackEnd::BuildDependenciesStorageInterface &m_buildDependenciesStorage;
};

struct Data // because we have a cycle dependency
{
    using TaskScheduler = ClangBackEnd::TaskScheduler<PchCreatorManager, ClangBackEnd::PchTaskQueue::Task>;

    Data(const QString &databasePath, const QString &pchsPath, const QString &preIncludeSearchPath)
        : database{Utils::PathString{databasePath}, 100000ms}
        , environment{pchsPath, preIncludeSearchPath}
    {}
    Sqlite::Database database;
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangBackEnd::FileSystem fileSystem{filePathCache};
    ClangPathWatcher<QFileSystemWatcher, QTimer> includeWatcher{filePathCache, fileSystem};
    ApplicationEnvironment environment;
    ProjectPartsStorage<> projectPartsStorage{database};
    PrecompiledHeaderStorage<> preCompiledHeaderStorage{database};
    GeneratedFiles generatedFiles;
    ProjectPartsManager projectParts{projectPartsStorage,
                                     preCompiledHeaderStorage,
                                     buildDependencyProvider,
                                     filePathCache,
                                     includeWatcher,
                                     generatedFiles};
    PchCreatorManager pchCreatorManager{generatedFiles,
                                        environment,
                                        filePathCache,
                                        clangPchManagerServer,
                                        includeWatcher,
                                        buildDependencyStorage};
    ClangBackEnd::ProgressCounter pchCreationProgressCounter{[&](int progress, int total) {
        executeInLoop([&] {
            clangPchManagerServer.setPchCreationProgress(progress, total);
        });
    }};
    ClangBackEnd::ProgressCounter dependencyCreationProgressCounter{[&](int progress, int total) {
        executeInLoop([&] {
            clangPchManagerServer.setDependencyCreationProgress(progress, total);
        });
    }};
    ClangBackEnd::PchTaskQueue pchTaskQueue{systemTaskScheduler,
                                            projectTaskScheduler,
                                            pchCreationProgressCounter,
                                            preCompiledHeaderStorage,
                                            database,
                                            environment,
                                            fileSystem,
                                            filePathCache};
    ClangBackEnd::PchTasksMerger pchTaskMerger{pchTaskQueue};
    ClangBackEnd::BuildDependenciesStorage<> buildDependencyStorage{database};
    ClangBackEnd::BuildDependencyCollector buildDependencyCollector{filePathCache,
                                                                    generatedFiles,
                                                                    environment};
    ClangBackEnd::ModifiedTimeChecker<ClangBackEnd::SourceEntries> modifiedTimeChecker{fileSystem};
    ClangBackEnd::BuildDependenciesProvider buildDependencyProvider{buildDependencyStorage,
                                                                    modifiedTimeChecker,
                                                                    buildDependencyCollector,
                                                                    database};
    ClangBackEnd::PchTaskGenerator pchTaskGenerator{buildDependencyProvider,
                                                    pchTaskMerger,
                                                    dependencyCreationProgressCounter,
                                                    pchTaskQueue};
    PchManagerServer clangPchManagerServer{includeWatcher,
                                           pchTaskGenerator,
                                           projectParts,
                                           generatedFiles,
                                           buildDependencyStorage,
                                           filePathCache};
    TaskScheduler systemTaskScheduler{pchCreatorManager,
                                      pchTaskQueue,
                                      pchCreationProgressCounter,
                                      std::thread::hardware_concurrency(),
                                      ClangBackEnd::CallDoInMainThreadAfterFinished::No};
    TaskScheduler projectTaskScheduler{pchCreatorManager,
                                       pchTaskQueue,
                                       pchCreationProgressCounter,
                                       std::thread::hardware_concurrency(),
                                       ClangBackEnd::CallDoInMainThreadAfterFinished::Yes};
};

#ifdef Q_OS_WIN
extern "C" void __stdcall OutputDebugStringW(const wchar_t* msg);
static void messageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    OutputDebugStringW(msg.toStdWString().c_str());
    std::wcout << msg.toStdWString() << std::endl;
    if (type == QtFatalMsg)
        abort();
}
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    qInstallMessageHandler(messageOutput);
#endif
    try {
        QCoreApplication::setOrganizationName(QStringLiteral("QtProject"));
        QCoreApplication::setOrganizationDomain(QStringLiteral("qt-project.org"));
        QCoreApplication::setApplicationName(QStringLiteral("ClangPchManagerBackend"));
        QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

        PchManagerApplication application(argc, argv);

        const QStringList arguments = processArguments(application);
        const QString connectionName = arguments[0];
        const QString databasePath = arguments[1];
        const QString pchsPath = arguments[2];
        const QString preIncludeSearchPath = arguments[3] + "/indexer_preincludes";

        Data data{databasePath, pchsPath, preIncludeSearchPath};

        data.includeWatcher.setNotifier(&data.clangPchManagerServer);

        ConnectionServer<PchManagerServer, PchManagerClientProxy> connectionServer;
        connectionServer.setServer(&data.clangPchManagerServer);
        data.buildDependencyProvider.setEnsureAliveMessageIsSentCallback(
            [&] { connectionServer.ensureAliveMessageIsSent(); });
        connectionServer.start(connectionName);

        return application.exec();
    } catch (const Sqlite::Exception &exception) {
        exception.printWarning();
    }
}


