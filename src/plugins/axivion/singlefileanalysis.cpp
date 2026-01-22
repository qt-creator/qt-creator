// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "singlefileanalysis.h"

#include "axivionperspective.h"
#include "axivionsettings.h"
#include "axiviontextmarks.h"
#include "axiviontr.h"
#include "axivionutils.h"
#include "localbuild.h"
#include "pluginarserver.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/projectmanager.h>

#include <QtTaskTree/QAbstractTaskTreeRunner>

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace QtTaskTree;
using namespace Utils;

namespace Axivion::Internal {

Q_LOGGING_CATEGORY(sfaLog, "qtc.axivion.sfa", QtWarningMsg)

class SingleFileDialog : public QDialog
{
public:
    SingleFileDialog()
    {
        bauhausConfig.setExpectedKind(PathChooser::ExistingDirectory);
        bauhausConfig.setAllowPathFromDevice(false);
        bauhausConfig.setHistoryCompleter("Axivion.SFABauhausConfig");
        command.setExpectedKind(PathChooser::Any);
        command.setAllowPathFromDevice(false);
        command.setHistoryCompleter("Axivion.SFACommand");

        QWidget *widget = new QWidget(this);
        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        auto okButton = buttons->button(QDialogButtonBox::Ok);
        okButton->setText(Tr::tr("Start"));
        auto hint = new QLabel(this);
        auto warning = new InfoLabel(Tr::tr("No active project. "
                                            "Referring %{ActiveProject:...} will fail."),
                                     InfoLabel::Warning, this);
        if (!ProjectExplorer::ProjectManager::projects().isEmpty())
            warning->setVisible(false);
        hint->setText(Tr::tr("build_compile_commands --single_file %{CurrentDocument:FilePath} "
                             "%{ActiveProject:BuildConfig:Path}/compile_commands.json\n"
                             "or some shell/batch script holding cafeCC / axivion_analysis commands"
                             " to execute."));
        // for now only build_compile_commands...
        // Makefile alternative..
        // ActiveProject may be empty if no project is opened or different from current Axivion's
        // projectName - do we have to handle this or just fail?
        using namespace Layouting;
        Column {
            Layouting::Group {
                title(Tr::tr("BAUHAUS_CONFIG Directory")), // could this be multiple directories?
                Column {
                    Row { Tr::tr("Usually the directory containing the axivion_config.json file.") },
                    Row { bauhausConfig }
                }
            },
            Layouting::Group {
                title(Tr::tr("Analysis Command")),
                Column {
                    Row { command },
                    Row { hint },
                    Row { warning }
                }
            },
            st
        }.attachTo(widget);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(widget);
        layout->addWidget(buttons);
        connect(okButton, &QPushButton::clicked,
                this, &QDialog::accept);
        connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
                this, &QDialog::reject);
        setWindowTitle(Tr::tr("Single File Analysis"));
        okButton->setEnabled(false);

        auto updateOk = [okButton, this] {
            okButton->setEnabled(bauhausConfig.pathChooser()->isValid() && !command().isEmpty());
        };
        connect(&bauhausConfig, &FilePathAspect::validChanged, this, updateOk);
        connect(&command, &FilePathAspect::changed, this, updateOk);

        resize(750, 300);
    }

    FilePathAspect bauhausConfig;
    FilePathAspect command;
};

struct SFAData
{
    FilePath file;
    FilePath bauhausSuite;
    FilePath bauhausConfig;
    FilePath analysisCommand;
    QString pipeName;
    int sessionId = -1;
};

class SingleFileAnalysis
{
public:
    SingleFileAnalysis() = default;
    ~SingleFileAnalysis()
    {
        QTC_CHECK(m_startedAnalyses.isEmpty()); // shutdownAll() must be done already
        QTC_CHECK(!m_startedAnalysesRunner.isRunning());
    }

    void startAnalysisFor(const FilePath &file, const QString &projectName,
                          const FilePath &bauhausConf, const FilePath &analysisCmd);
    void cancelAnalysisFor(const QString &fileName, const QString &projectName);
    void removeFinishedAnalyses();

    bool hasRunningAnalysisFor(const QString &projectName) const
    {
        return m_startedAnalysesRunner.isKeyRunning(projectName);
    }

    LocalBuildInfo localBuildInfoFor(const QString &projectName, const QString &/*fileName*/) const
    {
        return m_localBuildInfos.value(projectName);
    }

    void shutdownAll();

private:
    void onPluginArServerRunning(const QString &projectName);
    void onSessionStarted(const QString &projectName, int sessionId);

    QHash<QString, SFAData> m_startedAnalyses;
    QHash<QString, LocalBuildInfo> m_localBuildInfos;
    QMappedTaskTreeRunner<QString> m_startedAnalysesRunner;
};

void SingleFileAnalysis::startAnalysisFor(const FilePath &filePath, const QString &projectName,
                                          const FilePath &bauhausConf, const FilePath &analysisCmd)
{
    // start pluginarserver if not running
    // get bauhaus suite path and session id
    Environment env = Environment::systemEnvironment();
    FilePath bauhausSuite;
    if (settings().versionInfo())
        bauhausSuite = settings().axivionSuitePath();
    if (bauhausSuite.isEmpty())
        bauhausSuite = env.searchInPath("pluginARServer").parentDir().parentDir();
    if (bauhausSuite.isEmpty()) {
        QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Single File Analysis"),
                              Tr::tr("Axivion Suite path is neither configured nor in PATH.\n"
                                     "Set it up in Preferences > Analyzers > Axivion to start a "
                                     "single file analysis."));
        return;
    }
    SFAData data;
    data.file = filePath;
    data.bauhausSuite = bauhausSuite;
    data.bauhausConfig = bauhausConf;
    data.analysisCommand = analysisCmd;

    auto onServerRunning = [this, projectName] { onPluginArServerRunning(projectName); };
    showLocalBuildProgress();
    qCDebug(sfaLog) << "starting single file analysis for" << projectName << filePath;
    updateSfaStateFor(projectName, filePath.fileName(), Tr::tr("Preparing"), 5);
    m_startedAnalyses.insert(projectName, data);
    startPluginArServer(bauhausSuite, onServerRunning);
}

void SingleFileAnalysis::onPluginArServerRunning(const QString &projectName)
{
    QTC_ASSERT(m_startedAnalyses.contains(projectName), return);
    SFAData data = m_startedAnalyses.value(projectName);
    updateSfaStateFor(projectName, data.file.fileName(), Tr::tr("Preparing"), 10);
    requestArSessionStart(data.bauhausSuite, [this, projectName](int sessionId) {
        onSessionStarted(projectName, sessionId);
    });
}

static Environment setupEnv(const SFAData &data)
{
    Environment env = Environment::systemEnvironment();
    env.prependOrSetPath(data.bauhausSuite.pathAppended("bin"));
    if (!settings().javaHome().isEmpty())
        env.set("JAVA_HOME", settings().javaHome().toUserOutput());
    if (!settings().bauhausPython().isEmpty())
        env.set("BAUHAUS_PYTHON", settings().bauhausPython().toUserOutput());
    env.set("PYTHON_IO_ENCODING", "utf-8:replace");
    env.set("AXIVION_USER_AGENT", QString::fromUtf8(axivionUserAgent()));
    env.set("AXIVION_AR_PIPE_OUT", data.pipeName);
    env.set("BAUHAUS_CONFIG", data.bauhausConfig.toUserOutput());
    return env;
}

void SingleFileAnalysis::onSessionStarted(const QString &projectName, int sessionId)
{
    QTC_ASSERT(m_startedAnalyses.contains(projectName), return);
    SFAData &analysis = m_startedAnalyses[projectName];
    analysis.sessionId = sessionId;
    analysis.pipeName = pluginArPipeOut(analysis.bauhausSuite, sessionId);
    const Environment env = setupEnv(analysis);
    auto onSetup = [analysisCmd = analysis.analysisCommand, env](Process &process) {
        CommandLine cmd = HostOsInfo::isWindowsHost() ? CommandLine{"cmd", {"/c"}}
                                                      : CommandLine{"/bin/sh", {"-c"}};
        cmd.addCommandLineAsArgs(CommandLine{analysisCmd}, CommandLine::Raw);
        process.setCommand(cmd);
        process.setEnvironment(env);
    };
    const SFAData data = analysis;
    auto onDone = [data, projectName, this](const Process &process) {
        m_localBuildInfos.insert(projectName, {LocalBuildState::Finished,
                                               process.cleanedStdOut(), process.cleanedStdErr()});
        const QString resultStr = (process.result() == ProcessResult::FinishedWithSuccess)
                ? Tr::tr("Finished") : Tr::tr("Failed");
        updateSfaStateFor(projectName, data.file.fileName(), resultStr, 100);

        if (process.result() == ProcessResult::FinishedWithSuccess)
            qCDebug(sfaLog) << "Build succeeded";
        else
            qCDebug(sfaLog) << "Build failed" << process.error() << process.errorString();

        m_startedAnalyses.remove(projectName); // output still in localBuildInfos, rest now useless?
        qCDebug(sfaLog) << "requesting session finish";
        requestArSessionFinish(data.bauhausSuite, data.sessionId, false);
    };
    clearAllMarks(LineMarkerType::SFA);
    qCDebug(sfaLog) << "starting analysis cmd" << analysis.analysisCommand;
    m_startedAnalysesRunner.start(projectName, {ProcessTask(onSetup, onDone)});
    m_localBuildInfos.insert(projectName, {LocalBuildState::Building});
    updateSfaStateFor(projectName, data.file.fileName(), Tr::tr("Building"), 20);
}

void SingleFileAnalysis::cancelAnalysisFor(const QString &/*fileName*/, const QString &projectName)
{
    m_startedAnalysesRunner.cancelKey(projectName);
}

void SingleFileAnalysis::removeFinishedAnalyses()
{
    auto it = m_localBuildInfos.begin();
    while (it != m_localBuildInfos.end()) {
        if (it->state == LocalBuildState::Finished)
            it = m_localBuildInfos.erase(it);
        else
            ++it;
    }
}

void SingleFileAnalysis::shutdownAll()
{
    m_startedAnalysesRunner.reset();
    // shutdown pluginArServers as well
    shutdownAllPluginArServers();
}

SingleFileAnalysis s_sfaInstance;

void startSingleFileAnalysis(const FilePath &file, const QString &projectName)
{
    if (ExtensionSystem::PluginManager::isShuttingDown())
        return;

    QTC_ASSERT(!projectName.isEmpty(), return);
    QTC_ASSERT(!file.isEmpty(), return);

    if (hasRunningLocalBuild(projectName)) {
        QMessageBox::information(Core::ICore::dialogParent(), Tr::tr("Single File Analysis"),
                                 Tr::tr("There is a local build running for '%1'.\n"
                                        "Try again when this has finished."));
        return;
    }
    if (s_sfaInstance.hasRunningAnalysisFor(projectName)) {
        QMessageBox::information(Core::ICore::dialogParent(), Tr::tr("Single File Analysis"),
                                 Tr::tr("There is already a single file analysis running for '%1'."
                                        "\nTry again when this has finished."));
        return;
    }

    SingleFileDialog dia;
    if (dia.exec() != QDialog::Accepted)
        return;

    s_sfaInstance.startAnalysisFor(file, projectName, dia.bauhausConfig(), dia.command());
}


void removeFinishedAnalyses()
{
    s_sfaInstance.removeFinishedAnalyses();
}

void shutdownAllAnalyses()
{
    s_sfaInstance.shutdownAll();
}

bool hasRunningSingleFileAnalysis(const QString &projectName)
{
    return s_sfaInstance.hasRunningAnalysisFor(projectName);
}

void cancelSingleFileAnalysis(const QString &fileName, const QString &projectName)
{
    s_sfaInstance.cancelAnalysisFor(fileName, projectName);
}

LocalBuildInfo localBuildInfoFor(const QString &projectName, const QString &fileName)
{
    return s_sfaInstance.localBuildInfoFor(projectName, fileName);
}

} // namespace Axivion::Internal
