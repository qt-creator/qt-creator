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

#include <QtTaskTree/QMappedTaskTreeRunner>

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
        QWidget *widget = new QWidget(this);
        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        auto okButton = buttons->button(QDialogButtonBox::Ok);
        okButton->setText(Tr::tr("Start"));
        auto hint = new QLabel(this);
        auto warning = new InfoLabel(
            //: %1 is a Qt Creator variable string
            Tr::tr(
                "No active project. "
                "Referring to %1 will fail.")
                .arg("<code>%{ActiveProject:...}</code>"),
            InfoLabel::Warning,
            this);
        warning->setElideMode(Qt::ElideNone); // ensure HTML stuff is taken into account
        if (!ProjectExplorer::ProjectManager::projects().isEmpty())
            warning->setVisible(false);
        hint->setText(
            "build_compile_commands --single_file %{CurrentDocument:FilePath} "
            "%{ActiveProject:BuildConfig:Path}/compile_commands.json\n"
            //: the text is preceded by a command to execute
            + Tr::tr(
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
                    Row { Tr::tr("Usually the directory containing the file \"axivion_config.json\".") },
                    Row { settings().lastBauhausConfig }
                }
            },
            Layouting::Group {
                title(Tr::tr("Analysis Command")),
                Column {
                    Row { settings().lastSfaCommand },
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

        auto updateOk = [okButton] {
            okButton->setEnabled(settings().lastBauhausConfig.pathChooser()->isValid()
                                 && !settings().lastSfaCommand.volatileValue().isEmpty());
        };
        connect(&settings().lastBauhausConfig, &FilePathAspect::validChanged, this, updateOk);
        connect(&settings().lastSfaCommand, &FilePathAspect::volatileValueChanged, this, updateOk);

        resize(750, 300);
    }
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

    void startAnalysisFor(const FilePath &file,
                          const FilePath &bauhausConf, const FilePath &analysisCmd);
    void cancelAnalysisFor(const FilePath &fileName);
    void removeFinishedAnalyses();

    bool hasRunningAnalysisFor(const FilePath &filePath) const
    {
        return m_startedAnalysesRunner.isKeyRunning(filePath);
    }

    LocalBuildState localBuildStateFor(const FilePath &filePath) const
    {
        return m_localBuildInfos.value(filePath);
    }

    void shutdownAll();

private:
    void onPluginArServerRunning(const FilePath &filePath);
    void onSessionStarted(const FilePath &filePath, int sessionId);

    QHash<FilePath, SFAData> m_startedAnalyses;
    QHash<FilePath, LocalBuildState> m_localBuildInfos;
    QSet<FilePath> m_canceledAnalyses;
    QMappedTaskTreeRunner<FilePath> m_startedAnalysesRunner;
};

void SingleFileAnalysis::startAnalysisFor(const FilePath &filePath,
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

    auto onServerRunning = [this, filePath] { onPluginArServerRunning(filePath); };
    auto onFailed = [this, filePath] {
        m_startedAnalyses.remove(filePath);
        updateSfaStateFor(filePath, Tr::tr("Failed"), 100);
    };
    if (settings().saveOpenFiles()) {
        if (!saveModifiedFiles(QString{}))
            return;
    }

    showLocalBuildProgress();
    qCDebug(sfaLog) << "starting single file analysis for" << filePath;
    updateSfaStateFor(filePath, Tr::tr("Preparing"), 5);
    m_startedAnalyses.insert(filePath, data);
    startPluginArServer(bauhausSuite, onServerRunning, onFailed);
}

void SingleFileAnalysis::onPluginArServerRunning(const FilePath &filePath)
{
    QTC_ASSERT(m_startedAnalyses.contains(filePath), return);
    SFAData data = m_startedAnalyses.value(filePath);
    updateSfaStateFor(filePath, Tr::tr("Preparing"), 10);
    requestArSessionStart(data.bauhausSuite, [this, filePath](int sessionId) {
        onSessionStarted(filePath, sessionId);
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
    env.set("PYTHONUNBUFFERED", "1");
    env.set("AXIVION_USER_AGENT", QString::fromUtf8(axivionUserAgent()));
    env.set("AXIVION_AR_PIPE_OUT", data.pipeName);
    env.set("BAUHAUS_CONFIG", data.bauhausConfig.toUserOutput());
    return env;
}

void SingleFileAnalysis::onSessionStarted(const FilePath &filePath, int sessionId)
{
    QTC_ASSERT(m_startedAnalyses.contains(filePath), return);
    SFAData &analysis = m_startedAnalyses[filePath];
    analysis.sessionId = sessionId;
    analysis.pipeName = pluginArPipeOut(analysis.bauhausSuite, sessionId);
    const Environment env = setupEnv(analysis);
    auto onSetup = [analysisCmd = analysis.analysisCommand, env, filePath](Process &process) {
        CommandLine cmd = HostOsInfo::isWindowsHost() ? CommandLine{"cmd", {"/c"}}
                                                      : CommandLine{"/bin/sh", {"-c"}};
        cmd.addCommandLineAsArgs(CommandLine{analysisCmd}, CommandLine::Raw);
        process.setCommand(cmd);
        process.setEnvironment(env);
        process.setStdErrLineCallback([filePath](const QString &line) {
            appendSfaOutputFor(filePath, line, OutputFormat::StdErrFormat);
        });
        process.setStdOutLineCallback([filePath](const QString &line) {
            appendSfaOutputFor(filePath, line, OutputFormat::StdOutFormat);
        });
    };
    const SFAData data = analysis;
    auto onDone = [data, filePath, this](const Process &process) {
        m_localBuildInfos.insert(filePath, {LocalBuildState::Finished});
        QString resultStr;
        QString text;
        if (process.result() == ProcessResult::FinishedWithSuccess) {
            resultStr = Tr::tr("Finished");
            text = Tr::tr("Single file analysis finished successfully.");
        } else if (m_canceledAnalyses.remove(filePath)) {
            resultStr = Tr::tr("Canceled");
            text = Tr::tr("Single file analysis was canceled.");
        } else {
            resultStr = Tr::tr("Failed");
            text = Tr::tr("Single file analysis failed.");
            if (!process.errorString().isEmpty())
                text.append(' ').append(process.errorString());
        }
        text.append('\n');
        appendSfaOutputFor(filePath, text, OutputFormat::LogMessageFormat);
        updateSfaStateFor(filePath, resultStr, 100);

        if (process.result() == ProcessResult::FinishedWithSuccess)
            qCDebug(sfaLog) << "Build succeeded";
        else
            qCDebug(sfaLog) << "Build failed" << process.error() << process.errorString();

        m_startedAnalyses.remove(filePath); // output still in localBuildInfos, rest now useless?
        qCDebug(sfaLog) << "requesting session finish";
        requestArSessionFinish(data.bauhausSuite, data.sessionId, false);
    };

    clearAllMarks(LineMarkerType::SFA);
    resetSfaConsole(filePath);
    const QString startingText = Tr::tr("Starting single file analysis (%1)")
            .arg(filePath.toUserOutput()).append('\n');
    appendSfaOutputFor(filePath, startingText, OutputFormat::LogMessageFormat);
    qCDebug(sfaLog) << "starting analysis cmd" << analysis.analysisCommand;
    m_startedAnalysesRunner.start(filePath, {ProcessTask(onSetup, onDone)});
    m_localBuildInfos.insert(filePath, {LocalBuildState::Building});
    updateSfaStateFor(filePath, Tr::tr("Building"), 20);
}

void SingleFileAnalysis::cancelAnalysisFor(const FilePath &filePath)
{
    m_canceledAnalyses.insert(filePath);
    m_startedAnalysesRunner.cancelKey(filePath);
}

void SingleFileAnalysis::removeFinishedAnalyses()
{
    auto it = m_localBuildInfos.begin();
    while (it != m_localBuildInfos.end()) {
        if (*it == LocalBuildState::Finished)
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

void startSingleFileAnalysis(const FilePath &file)
{
    if (ExtensionSystem::PluginManager::isShuttingDown())
        return;

    QTC_ASSERT(!file.isEmpty(), return);

    if (s_sfaInstance.hasRunningAnalysisFor(file)) {
        QMessageBox::information(
            Core::ICore::dialogParent(),
            Tr::tr("Single File Analysis"),
            Tr::tr(
                "There already is a single file analysis running for \"%1\"."
                "\nTry again when it has finished.").arg(file.toUserOutput()));
        return;
    }

    SingleFileDialog dia;
    if (dia.exec() != QDialog::Accepted)
        return;

    settings().lastBauhausConfig.apply();
    settings().lastSfaCommand.apply();
    settings().writeSettings();

    s_sfaInstance.startAnalysisFor(file, settings().lastBauhausConfig(), settings().lastSfaCommand());
}


void removeFinishedAnalyses()
{
    s_sfaInstance.removeFinishedAnalyses();
}

void shutdownAllAnalyses()
{
    s_sfaInstance.shutdownAll();
}

void cancelSingleFileAnalysis(const FilePath &filePath)
{
    s_sfaInstance.cancelAnalysisFor(filePath);
}

LocalBuildState localBuildStateFor(const FilePath &filePath)
{
    return s_sfaInstance.localBuildStateFor(filePath);
}

} // namespace Axivion::Internal
