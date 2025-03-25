// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "localbuild.h"

#include "axivionperspective.h"
#include "axivionsettings.h"
#include "axiviontr.h"

#include <extensionsystem/pluginmanager.h>

#include <solutions/tasking/tasktree.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QTimer>

#include <unordered_map>

using namespace Tasking;
using namespace Utils;

namespace Axivion::Internal {

Q_LOGGING_CATEGORY(sqlLog, "qtc.axivion.sql", QtWarningMsg)
Q_LOGGING_CATEGORY(localDashLog, "qtc.axivion.localdashboard", QtWarningMsg)

struct LocalDashboard
{
    QString id;
    CommandLine startCommandLine;
    CommandLine stopCommandLine;
    Environment environment;

    // access data will be updated after successful start
    QUrl localUrl;
    QString localUser;
    QString localProject;
    QByteArray pass;
};

class LocalBuild
{
public:
    LocalBuild() {}
    ~LocalBuild()
    {
        QTC_CHECK(m_startedDashboards.isEmpty()); // shutdownAll() must be done already
        QTC_CHECK(m_startedDashboardTrees.empty());
    }

    void startDashboard(const QString &projectName, const LocalDashboard &dashboard,
                        const std::function<void()> &callback);
    bool shutdownAll(const std::function<void()> &callback);

private:
    QHash<QString, LocalDashboard> m_startedDashboards;
    std::unordered_map<QString, std::unique_ptr<TaskTree>> m_startedDashboardTrees;
};

void LocalBuild::startDashboard(const QString &projectName, const LocalDashboard &dashboard,
                                const std::function<void()> &callback)
{
    if (ExtensionSystem::PluginManager::isShuttingDown())
        return;

    const Storage<LocalDashboard> storage;
    const auto onSetup = [dash = dashboard](Process &process) {
        process.setCommand(dash.startCommandLine);
        process.setEnvironment(dash.environment);
    };

    TaskTree *taskTree = new TaskTree;
    m_startedDashboardTrees.insert_or_assign(projectName, std::unique_ptr<TaskTree>(taskTree));
    const auto onDone = [this, callback, dash = dashboard, projectName] (const Process &process) {
        const auto onFinish = qScopeGuard([this, projectName] {
            auto it = m_startedDashboardTrees.find(projectName);
            QTC_ASSERT(it != m_startedDashboardTrees.end(), return);
            it->second.release()->deleteLater();
            m_startedDashboardTrees.erase(it);
        });
        if (process.result() != ProcessResult::FinishedWithSuccess) {
            qCDebug(localDashLog) << "Process failed...." << int(process.result());
            const QString errOutput = process.cleanedStdErr();
            if (errOutput.isEmpty())
                showErrorMessage(Tr::tr("Failed to start local dashboard."));
            else
                showErrorMessage(errOutput);
            return;
        }
        const QString output = process.cleanedStdOut();
        QJsonParseError error;
        const QJsonDocument json = QJsonDocument::fromJson(output.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
            return;
        if (!json.isObject())
            return;

        LocalDashboard updated = dash;
        const QJsonObject data = json.object();
        updated.localUrl = QUrl::fromUserInput(data.value("url").toString());
        updated.localProject = data.value("project").toString();
        updated.localUser = data.value("user").toString();
        updated.pass = data.value("password").toString().toUtf8();

        m_startedDashboards.insert(updated.id, updated);
        if (callback)
            callback();
    };

    m_startedDashboards.insert(dashboard.id, dashboard);
    qCDebug(localDashLog) << "Dashboard [start]" << dashboard.startCommandLine.toUserOutput();
    taskTree->setRecipe({ProcessTask(onSetup, onDone)});
    taskTree->start();
}

bool LocalBuild::shutdownAll(const std::function<void()> &callback)
{
    for (auto it = m_startedDashboardTrees.begin(), end = m_startedDashboardTrees.end();
         it != end; ++it) {
        if (it->second)
            it->second->cancel();
    }

    if (m_startedDashboards.isEmpty())
        return false;

    const LoopList iterator(m_startedDashboards.values());

    const auto onSetup = [iterator](Process &process) {
        process.setCommand(iterator->stopCommandLine);
        process.setEnvironment(iterator->environment);
        qCDebug(localDashLog) << "Dashboard [stop]" << iterator->stopCommandLine.toUserOutput();
    };

    const auto onDone = [this, iterator](const Process &) {
        m_startedDashboards.remove(iterator->id);
    };

    const Group recipe = Group {
        For (iterator) >> Do {
            parallel,
            continueOnError,
            ProcessTask(onSetup, onDone)
        }
    }.withTimeout(std::chrono::seconds(5));

    TaskTree *taskTree = new TaskTree(recipe);
    QObject::connect(taskTree, &TaskTree::done, taskTree, [taskTree, callback] {
        taskTree->deleteLater();
        if (callback)
            callback();
    });
    taskTree->start();

    return true;
}

LocalBuild s_localBuildInstance;

static QSqlDatabase localDashboardDB()
{
    static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "localDashboardDB");
    return db;
}

void checkForLocalBuildResults(const QString &projectName, const std::function<void()> &callback)
{
    const FilePath configDbl = FileUtils::homePath().pathAppended(".bauhaus/localbuild/config.dbl");
    if (!configDbl.exists())
        return;

    if (!QSqlDatabase::isDriverAvailable("QSQLITE"))
        return;

    QSqlDatabase db = localDashboardDB();
    if (!db.isValid())
        return;
    db.setDatabaseName(configDbl.path());
    if (!db.open()) {
        qCDebug(sqlLog) << "open db failed" << db.lastError().text();
        return;
    }
    auto cleanup = qScopeGuard([&db] { db.close(); });

    QSqlQuery query(db);
    query.prepare("SELECT Data FROM axMetaData WHERE Name=\"version\"");
    if (!query.exec() || !query.next())
        return;
    if (!query.value("Data").toString().startsWith("1."))
        return;

    query.prepare("SELECT COUNT(*) FROM axLocalProjects WHERE Remote_Project_Name=(:projectName)");
    query.bindValue(":projectName", projectName);

    if (!query.exec() || !query.next())
        return;
    bool ok = true;
    const int count = query.value(0).toUInt(&ok);
    if (!ok || count < 1)
        return;

    if (callback)
        callback();
}

static CommandLine parseCommandLine(const QString &jsonArrayCmd)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonArrayCmd.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError)
        return {};
    if (!doc.isArray())
        return {};
    const QJsonArray array = doc.array();
    QStringList fullCommand;
    for (const auto &val : array)
        fullCommand.append(val.toString());
    if (fullCommand.isEmpty())
        return {};
    const QString first = fullCommand.takeFirst();
    return CommandLine{FilePath::fromUserInput(first).withExecutableSuffix(), fullCommand};
}

void startLocalDashboard(const QString &projectName, const std::function<void ()> &callback)
{
    QSqlDatabase db = localDashboardDB();
    QTC_ASSERT(db.isValid(), return); // we should be here only if we had some valid db before
    if (!db.open()) {
        qCDebug(sqlLog) << "open db failed" << db.lastError().text();
        return;
    }
    auto cleanup = qScopeGuard([&db]{ db.close(); });

    QSqlQuery query(db);
    query.prepare("SELECT ID, Dashboard_Start_Command_Line, Dashboard_Stop_Command_Line "
                  "FROM axLocalProjects WHERE Remote_Project_Name=(:projectName)");
    query.bindValue(":projectName", projectName);
    if (!query.exec() || !query.next())
        return;

    const QString id = query.value("ID").toString();
    const QString startCmdLine = query.value("Dashboard_Start_Command_Line").toString();
    const QString stopCmdLine = query.value("Dashboard_Stop_Command_Line").toString();

    query.prepare("SELECT Name, Value FROM axDashboardEnvironments WHERE LocalProject_ID=(:id)");
    query.bindValue(":id", id);

    if (!query.exec())
        return;

    const QString userAgent("Axivion" + QCoreApplication::applicationName() +
                            "Plugin/" + QCoreApplication::applicationVersion());
    EnvironmentItems envItems;
    if (!settings().bauhausPython().isEmpty())
        envItems.append(EnvironmentItem("BAUHAUS_PYTHON", settings().bauhausPython().path()));
    if (!settings().javaHome().isEmpty())
        envItems.append(EnvironmentItem("JAVA_HOME", settings().javaHome().path()));
    envItems.append(EnvironmentItem("AXIVION_USER_AGENT", userAgent));
    while (query.next()) {
        const QString name = query.value("Name").toString();
        const QString value = query.value("Value").toString();
        QTC_ASSERT(!name.isEmpty(), continue);
        envItems.append(EnvironmentItem(name, value));
    }

    Environment env = Environment::systemEnvironment();
    env.modify(envItems);
    const CommandLine start = parseCommandLine(startCmdLine);
    const CommandLine stop = parseCommandLine(stopCmdLine);

    LocalDashboard localDashboard{id, start, stop, env, {}, {}, {}, {}};
    s_localBuildInstance.startDashboard(projectName, localDashboard, callback);
}

bool shutdownAllLocalDashboards(const std::function<void()> &callback)
{
    return s_localBuildInstance.shutdownAll(callback);
}

} // namespace Axivion::Internal
