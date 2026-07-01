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

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/makestep.h>
#include <projectexplorer/project.h>
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

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Axivion::Internal {

static Q_LOGGING_CATEGORY(sfaLog, "qtc.axivion.sfa", QtWarningMsg)

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
        if (!ProjectManager::projects().isEmpty())
            warning->setVisible(false);
        hint->setText(
            "build_compile_commands --single_file %{CurrentDocument:FilePath} "
            "%{ActiveProject:BuildConfig:Path}/compile_commands.json\n"
            //: the text is preceded by a command to execute
            + Tr::tr(
                "or some shell/batch script holding cafeCC / axivion_analysis commands"
                " to execute.")
                    .append("\n\n")
                    .append(Tr::tr("Leave empty to derive from active project. File to analyze "
                                   "must be part of the active project.")));
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

        const bool canDerive = buildInfoForCurrentDocumentDerivable();
        auto updateOk = [okButton, canDerive] {
            okButton->setEnabled(settings().lastBauhausConfig.pathChooser()->isValid()
                                 && (canDerive
                                     || !settings().lastSfaCommand.volatileValue().isEmpty()));
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
    FilePath workingDirectory;
    // system environment for manually specified command; active build configuration's
    // build environment for derived analyses
    Environment baseEnvironment = Environment::systemEnvironment();
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

    void startAnalysisFor(const FilePath &file, const FilePath &bauhausConf,
                          const FilePath &analysisCmd, const Environment &baseEnv,
                          const FilePath &workingDir);
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

void SingleFileAnalysis::startAnalysisFor(const FilePath &filePath, const FilePath &bauhausConf,
                                          const FilePath &analysisCmd, const Environment &baseEnv,
                                          const FilePath &workingDir)
{
    // start pluginarserver if not running
    // get bauhaus suite path and session id
    Environment env = baseEnv;
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
    data.baseEnvironment = baseEnv;
    data.workingDirectory = workingDir;

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
    // system environment for manually specified commands, build environment for derived
    // commands, so that PATH and user defined variables which might be used by
    // the analysis commands are available
    Environment env = data.baseEnvironment;
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

    const SFAData data = analysis;
    auto onSetup = [data, env, filePath](Process &process) {
        CommandLine cmd = HostOsInfo::isWindowsHost() ? CommandLine{"cmd", {"/c"}}
                                                      : CommandLine{"/bin/sh", {"-c"}};
        cmd.addCommandLineAsArgs(CommandLine{data.analysisCommand}, CommandLine::Raw);
        process.setCommand(cmd);
        process.setEnvironment(env);
        if (!data.workingDirectory.isEmpty())
            process.setWorkingDirectory(data.workingDirectory);
        process.setStdErrLineCallback([filePath](const QString &line) {
            appendSfaOutputFor(filePath, line, OutputFormat::StdErrFormat);
        });
        process.setStdOutLineCallback([filePath](const QString &line) {
            appendSfaOutputFor(filePath, line, OutputFormat::StdOutFormat);
        });
    };
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
            qCDebug(sfaLog) << "Build failed" << int(process.error()) << process.errorString();

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

static FilePath constructMakefileIntegrationCommand(const FilePath &file,
                                                    const FilePath &makefile,
                                                    MakeStep *ms)
{
    const bool isWin = HostOsInfo::isWindowsHost();
    const QString fileObj = ProcessArgs::quoteArg(file.completeBaseName().append(isWin ? ".obj"
                                                                                       : ".o"));

    const QStringList cmds = {
        ProcessArgs::quoteArg(ms->makeExecutable().toUserOutput())
            + " CC=cafeCC CXX=cafeCC -f "
            + ProcessArgs::quoteArg(makefile.toUserOutput()) + ' ' + fileObj, // make
        "axivion_analysis --output - --quiet --limit_to AnalysisControl"
        " --limit_to Stylechecks --ir " + fileObj                             // analysis
    };
    return FilePath::fromString(cmds.join(" && "));
}

static void startSingleFileAnalysisDerived(const FilePath &file)
{
    Project *startupProject = ProjectManager::startupProject();
    Project *project = ProjectManager::projectForFile(file);
    BuildSystem *bs = startupProject && startupProject == project ? project->activeBuildSystem()
                                                                  : nullptr;
    if (bs) {

        BuildConfiguration *bc = bs->buildConfiguration();
        if (bs->name() == "cmake" && bc) {
            // Run cmake to (re)generate compile_commands.json in the build directory (this
            // overwrites an already present one), then run build_compile_commands on it.
            const FilePath buildDir = bc->buildDirectory();
            const FilePath compileCommandsJson = buildDir.pathAppended("compile_commands.json");

            const QStringList commands = {
                // cmake call to generate compile_commands.json
                ProcessArgs::quoteArg(bs->activeBuildTool().toUserOutput())
                + " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .",
                // build and run analysis
                "build_compile_commands --single_file "
                + ProcessArgs::quoteArg(file.toUserOutput()) + ' '
                + ProcessArgs::quoteArg(compileCommandsJson.toUserOutput())
            };
            const FilePath cmd = FilePath::fromString(commands.join(" && "));

            s_sfaInstance.startAnalysisFor(file, settings().lastBauhausConfig(), cmd,
                                           bc->environment(), buildDir);
            return;
        }

        if (bc) {
            // make integration - check for a makestep and make use of its make path
            // and its original Makefile, create secondary parallel build folder for
            // the sfa to avoid polluting the original build dir
            if (auto bst = bc->buildSteps()) {
                for (auto stp : bst->steps()) {
                    if (auto ms = qobject_cast<MakeStep *>(stp)) {
                        const FilePath makefile = bc->buildDirectory().pathAppended("Makefile");
                        if (!makefile.exists())
                            continue;

                        const FilePath axivionBuildDir = bc->buildDirectory()
                                .stringAppended("_axivionSFA");
                        if (!axivionBuildDir.ensureWritableDir())
                            break;
                        const FilePath objFile = axivionBuildDir.pathAppended(
                                    file.completeBaseName().append(HostOsInfo::isWindowsHost()
                                                                   ? ".obj" : ".o"));
                        if (objFile.exists()) {
                            if (auto result = objFile.removeFile(); !result)
                                break;
                        }

                        const FilePath cmd
                                = constructMakefileIntegrationCommand(file, makefile, ms);
                        s_sfaInstance.startAnalysisFor(file, settings().lastBauhausConfig(),
                                                       cmd, bc->environment(), axivionBuildDir);
                        return;
                    }
                }
            }
        }
        QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Single File Analysis"),
                              Tr::tr("Could not derive the commands for single file analysis "
                                     "automatically.\nYou need to specify the commands on your own."));
    }
}

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

    if (settings().lastSfaCommand().isEmpty()) {
        startSingleFileAnalysisDerived(file);
    } else {
        s_sfaInstance.startAnalysisFor(file, settings().lastBauhausConfig(),
                                       settings().lastSfaCommand(),
                                       Environment::systemEnvironment(), FilePath{});
    }
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
