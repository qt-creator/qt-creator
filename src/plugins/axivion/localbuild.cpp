// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "localbuild.h"

#include "axivionperspective.h"
#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/idocument.h>

#include <extensionsystem/pluginmanager.h>

#include <solutions/tasking/tasktreerunner.h>
#include <solutions/tasking/tasktree.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLoggingCategory>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QTimer>
#include <QVBoxLayout>

#include <unordered_map>

using namespace Tasking;
using namespace Utils;

namespace Axivion::Internal {

Q_LOGGING_CATEGORY(sqlLog, "qtc.axivion.sql", QtWarningMsg)
Q_LOGGING_CATEGORY(localDashLog, "qtc.axivion.localdashboard", QtWarningMsg)
Q_LOGGING_CATEGORY(localBuildLog, "qtc.axivion.localbuild", QtWarningMsg)

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
        QTC_ASSERT(m_runningLocalBuilds.isEmpty(), qDeleteAll(m_runningLocalBuilds));
        QTC_CHECK(m_startedDashboardTrees.empty());
    }

    std::optional<LocalDashboardAccess> localDashboardAccessFor(const QString &projectName) const;

    void startDashboard(const QString &projectName, const LocalDashboard &dashboard,
                        const std::function<void()> &callback);
    bool shutdownAll(const std::function<void()> &callback);

    bool startLocalBuildFor(const QString &projectName);
    void cancelLocalBuildFor(const QString &projectName);

    bool hasRunningBuildFor(const QString &projectName)
    {
        return m_runningLocalBuilds.contains(projectName);
    }

    LocalBuildInfo localBuildInfoFor(const QString &projectName)
    {
        return m_localBuildInfos.value(projectName);
    }

    void removeFinishedLocalBuilds();

    FilePath lastBauhausBase() const { return m_lastBauhausFromDB; }
    void setLastBauhausFromDatabase(const FilePath &bauhausDir)
    {
        m_lastBauhausFromDB = bauhausDir;
    }

private:
    void handleLocalBuildOutputFor(const QString &projectName, const QString &line);

    QHash<QString, LocalDashboard> m_startedDashboards;
    std::unordered_map<QString, std::unique_ptr<TaskTree>> m_startedDashboardTrees;

    QHash<QString, TaskTreeRunner *> m_runningLocalBuilds;
    QHash<QString, LocalBuildInfo> m_localBuildInfos;
    FilePath m_lastBauhausFromDB;
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
    for (auto it : std::as_const(m_runningLocalBuilds)) // runners that perform a local build
        it->cancel();

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

std::optional<LocalDashboardAccess> LocalBuild::localDashboardAccessFor(
        const QString &projectName) const
{
    const LocalDashboard found
            = Utils::findOrDefault(m_startedDashboards, [projectName](const LocalDashboard &d) {
        return d.localProject == projectName;
    });
    if (found.localProject.isEmpty())
        return std::nullopt;

    LocalDashboardAccess result{found.localUser,
                                QString::fromUtf8(found.pass),
                                found.localUrl.toString(QUrl::None)};
    return std::make_optional(result);
}

LocalBuild s_localBuildInstance;

static QSqlDatabase localDashboardDB()
{
    static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "localDashboardDB");
    return db;
}

void checkForLocalBuildResults(const QString &projectName, const std::function<void()> &callback)
{
    s_localBuildInstance.setLastBauhausFromDatabase({}); // clear old

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

    query.prepare("SELECT Bauhaus_Base_Dir FROM axLocalProjects WHERE Remote_Project_Name=(:projectName)");
    query.bindValue(":projectName", projectName);
    if (query.exec() && query.next()) {
        FilePath bauhaus = FilePath::fromUserInput(query.value("Bauhaus_Base_Dir").toString());
        s_localBuildInstance.setLastBauhausFromDatabase(bauhaus);
        qCDebug(sqlLog) << "set bauhaus base from DB" << bauhaus.toUserOutput();
    }

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

class LocalBuildDialog : public QDialog
{
public:
    LocalBuildDialog(const QString &projectName)
    {
        bauhausSuite.setExpectedKind(PathChooser::ExistingDirectory);
        bauhausSuite.setAllowPathFromDevice(false);
        if (!s_localBuildInstance.lastBauhausBase().isEmpty())
            bauhausSuite.setValue(s_localBuildInstance.lastBauhausBase());
        else if (settings().versionInfo())
            bauhausSuite.setValue(settings().axivionSuitePath());
        fileOrCommand.setExpectedKind(PathChooser::Any);
        fileOrCommand.setAllowPathFromDevice(false);
        fileOrCommand.setHistoryCompleter("LocalBuildHistory");
        if (!settings().lastLocalBuildCommand().isEmpty())
            fileOrCommand.setValue(settings().lastLocalBuildCommand());
        buildType.setLabelText(Tr::tr("Build type:"));
        buildType.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
        buildType.setToolTip(Tr::tr("Clean Build: Set environment variable AXIVION_CLEAN_BUILD=1\n"
                                    "Incremental Build: Set environment variable AXIVION_INCREMENTAL_BUILD=1"));
        buildType.addOption("");
        buildType.addOption(Tr::tr("Clean Build"));
        buildType.addOption(Tr::tr("Incremental Build"));

        QWidget *widget = new QWidget(this);

        auto warn1 = new QLabel(widget);
        warn1->setPixmap(Icons::WARNING.pixmap());
        warn1->setAlignment(Qt::AlignTop);
        auto warnText1 = new QLabel(Tr::tr("Warning: Modifying source files during the local build may "
                                           "produce unexpected warnings, errors, or wrong results."),
                                    widget);
        warnText1->setAlignment(Qt::AlignLeft);
        warnText1->setWordWrap(true);
        auto warn2 = new QLabel(widget);
        warn2->setAlignment(Qt::AlignTop);
        warn2->setPixmap(Icons::WARNING.pixmap());
        auto warnText2 = new QLabel(Tr::tr("Warning: If your build is not configured for local build, you "
                                           "may overwrite output files of your native compiler when starting "
                                           "a local build."), widget);
        warnText2->setAlignment(Qt::AlignLeft);
        warnText2->setWordWrap(true);
        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, this);
        auto okButton = buttons->button(QDialogButtonBox::Ok);
        okButton->setText(Tr::tr("Start Local Build"));
        okButton->setEnabled(false);

        using namespace Layouting;
        Column {
            Row {
                Column {
                    Form {
                       warn1, warnText1, br,
                       warn2, warnText2, br,
                   },
                   Space(20),
                   Tr::tr("Choose the same Axivion Suite version as your CI build uses "
                          "or the results may differ.")
                }, st
            }, st,
            Row { Tr::tr("Axivion Suite installation directory:") },
            Row { bauhausSuite },
            Row { Tr::tr("Enter the command for building %1:").arg(projectName) },
            Row { fileOrCommand },
            Row { buildType, st },
            st
        }.attachTo(widget);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(widget);
        layout->addWidget(buttons);

        connect(&fileOrCommand, &FilePathAspect::changed,
                this, [this, okButton] { okButton->setEnabled(!fileOrCommand().isEmpty()); });
        connect(okButton, &QPushButton::clicked,
                this, &QDialog::accept);
        connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
                this, &QDialog::reject);
        setWindowTitle(Tr::tr("Local Build Command: %1").arg(projectName));
        okButton->setEnabled(!fileOrCommand().isEmpty());
    }

    FilePathAspect bauhausSuite;
    FilePathAspect fileOrCommand;
    SelectionAspect buildType;
};

void LocalBuild::handleLocalBuildOutputFor(const QString &projectName, const QString &line)
{
    static const QRegularExpression buildStateRegex(
                R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} - (?<type>[a-z ]{4}) - (.+) - (?<text>.+)$)");
    QTC_ASSERT(hasRunningBuildFor(projectName), return);
    const LocalBuildState state = m_localBuildInfos.value(projectName).state;

    const QRegularExpressionMatch match = buildStateRegex.match(line);
    switch (state) {
    case LocalBuildState::None:
        m_localBuildInfos.insert(projectName, {LocalBuildState::Started});
        qCDebug(localBuildLog) << "buildState changed > started" << projectName;
        updateLocalBuildStateFor(projectName, Tr::tr("Started"), 5);
        break;
    case LocalBuildState::Started:
        if (match.hasMatch()) {
            const QString type = match.captured("type").trimmed();
            if (type == "sql" && match.captured("text").startsWith("Finished import")) {
                qCDebug(localBuildLog) << "local dashboard changed.. (LocalBuild)"; // TODO trigger update of perspective?
            } else if (type == "bld") {
                m_localBuildInfos.insert(projectName, {LocalBuildState::Building});
                qCDebug(localBuildLog) << "buildState changed > building" << projectName;
                updateLocalBuildStateFor(projectName, Tr::tr("Building"), 30);
            }
        }
        break;
    case LocalBuildState::Building:
        if (match.hasMatch()) {
            if (match.captured("type").trimmed() == "bld"
                    && match.captured("text").startsWith("End of build actions")) {
                m_localBuildInfos.insert(projectName, {LocalBuildState::Analyzing});
                qCDebug(localBuildLog) << "buildState changed > analyzing" << projectName;
                updateLocalBuildStateFor(projectName, Tr::tr("Analyzing"), 60);
            }
        }
        break;
    case LocalBuildState::Analyzing:
        if (match.hasMatch()) {
            const QString type = match.captured("type").trimmed();
            if (type == "sql" || "db") {
                m_localBuildInfos.insert(projectName, {LocalBuildState::UpdatingDashboard});
                qCDebug(localBuildLog) << "buildState changed > updatingdashboard" << projectName;
                updateLocalBuildStateFor(projectName, Tr::tr("Updating Dashboard"), 90);
            }
        }
        break;
    case LocalBuildState::UpdatingDashboard:
    case LocalBuildState::Finished:
        break;
    }

}

static void setupEnvAndCommandLineFromUserInput(Environment *env, CommandLine *cmdLine,
                                                const FilePath &file, int buildType)
{
    switch (buildType) {
    case 1:
        env->set("AXIVION_CLEAN_BUILD", "1");
        break;
    case 2:
        env->set("AXIVION_INCREMENTAL_BUILD", "1");
        break;
    default:
        break;
    }

    if (file.isDir() || (file.isFile() && file.suffix() == "json")) {
        env->set("BAUHAUS_CONFIG", file.toUserOutput());
        *cmdLine = CommandLine{ FilePath("axivion_ci").withExecutableSuffix() };
    } else {
        *cmdLine = CommandLine{file};
    }
}

static bool saveModifiedFiles(const QString &projectName)
{
    QList<Core::IDocument *> modifiedDocs = Core::DocumentManager::modifiedDocuments();
    if (modifiedDocs.isEmpty())
        return true;

    // if we have a mapping, limit to docs of this project directory, otherwise save all
    const FilePath projectBase = settings().localProjectForProjectName(projectName);
    if (!projectBase.isEmpty()) {
        modifiedDocs = Utils::filtered(modifiedDocs, [projectBase](Core::IDocument *doc) {
                return doc->filePath().isChildOf(projectBase);
        });
    }
    bool canceled = false;
    bool success = Core::DocumentManager::saveModifiedDocumentsSilently(modifiedDocs, &canceled);
    return success && !canceled;
}

bool LocalBuild::startLocalBuildFor(const QString &projectName)
{
    if (ExtensionSystem::PluginManager::isShuttingDown())
        return false;

    QTC_ASSERT(!projectName.isEmpty(), return false);

    LocalBuildDialog dia(projectName);
    if (dia.exec() != QDialog::Accepted)
        return false;

    settings().lastLocalBuildCommand.setValue(dia.fileOrCommand());
    settings().writeSettings();

    Environment env = Environment::systemEnvironment();
    updateEnvironmentForLocalBuild(&env);
    if (!env.hasKey("AXIVION_LOCAL_BUILD"))
        return false;
    if (settings().saveOpenFiles()) {
        if (!saveModifiedFiles(projectName))
            return false;
    }

    const QString createdPassFile = env.value("AXIVION_PASSFILE");
    qCDebug(localDashLog) << "passfile:" << createdPassFile;

    CommandLine cmdLine;
    setupEnvAndCommandLineFromUserInput(&env, &cmdLine, dia.fileOrCommand(), dia.buildType());

    const FilePath bauhaus = dia.bauhausSuite();
    if (!bauhaus.isEmpty()) {
        env.set("AXIVION_BASE_DIR", bauhaus.toUserOutput());
        env.prependOrSetPath(bauhaus.pathAppended("bin"));
    }
    if (!settings().javaHome().isEmpty())
        env.set("JAVA_HOME", settings().javaHome().toUserOutput());
    if (!settings().bauhausPython().isEmpty())
        env.set("BAUHAUS_PYTHON", settings().bauhausPython().toUserOutput());

    TaskTreeRunner *localBuildRunner = new TaskTreeRunner;
    m_runningLocalBuilds.insert(projectName, localBuildRunner);

    const auto onSetup = [this, projectName, cmdLine, env](Process &process) {
        CommandLine cmd = HostOsInfo::isWindowsHost() ? CommandLine{"cmd", {"/c"}}
                                                      : CommandLine{"/bin/sh", {"-c"}};
        cmd.addCommandLineAsArgs(cmdLine, CommandLine::Raw);
        process.setCommand(cmd);
        process.setEnvironment(env);
        process.setUseCtrlCStub(true);

        process.setStdErrCallback([this, projectName](const QString &line) {
            handleLocalBuildOutputFor(projectName, line);
        });
    };

    const auto onDone = [this, projectName, createdPassFile](const Process &process) {
        const FilePath fp = FilePath::fromUserInput(createdPassFile);
        if (QTC_GUARD(fp.exists())) {
            fp.removeFile();
            qCDebug(localBuildLog) << "removed passfile: " << createdPassFile;
        }
        const QString state = process.result() == ProcessResult::FinishedWithSuccess
                ? Tr::tr("Finished") : Tr::tr("Failed");
        m_localBuildInfos.insert(projectName, {LocalBuildState::Finished, process.cleanedStdOut(),
                                               process.cleanedStdErr()});
        qCDebug(localBuildLog) << "buildState changed >" << state << projectName;
        TaskTreeRunner *runner = m_runningLocalBuilds.take(projectName);
        if (runner)
            runner->deleteLater();
        updateLocalBuildStateFor(projectName, state, 100);
    };

    m_localBuildInfos.insert(projectName, {LocalBuildState::None});
    updateLocalBuildStateFor(projectName, Tr::tr("Starting"), 1);
    qCDebug(localBuildLog) << "starting local build (" << projectName << "):"
                           << cmdLine.toUserOutput();
    localBuildRunner->start({ProcessTask(onSetup, onDone)});
    return true;
}

void LocalBuild::cancelLocalBuildFor(const QString &projectName)
{
    TaskTreeRunner *runner = m_runningLocalBuilds.value(projectName);
    if (runner)
        runner->cancel();
}

void LocalBuild::removeFinishedLocalBuilds()
{
    auto it = m_localBuildInfos.begin();
    while (it != m_localBuildInfos.end()) {
        if (it->state == LocalBuildState::Finished)
            it = m_localBuildInfos.erase(it);
        else
            ++it;
    }
}

bool shutdownAllLocalDashboards(const std::function<void()> &callback)
{
    return s_localBuildInstance.shutdownAll(callback);
}

std::optional<LocalDashboardAccess> localDashboardAccessFor(const QString &projectName)
{
    return s_localBuildInstance.localDashboardAccessFor(projectName);
}

bool startLocalBuild(const QString &projectName)
{
    return s_localBuildInstance.startLocalBuildFor(projectName);
}

void cancelLocalBuild(const QString &projectName)
{
    s_localBuildInstance.cancelLocalBuildFor(projectName);
}

bool hasRunningLocalBuild(const QString &projectName)
{
    return s_localBuildInstance.hasRunningBuildFor(projectName);
}

LocalBuildInfo localBuildInfoFor(const QString &projectName)
{
    return s_localBuildInstance.localBuildInfoFor(projectName);
}

void removeFinishedLocalBuilds()
{
    s_localBuildInstance.removeFinishedLocalBuilds();
}

} // namespace Axivion::Internal
