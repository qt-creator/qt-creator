// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonrunconfiguration.h"

#include "pipsupport.h"
#include "pyside.h"
#include "pysidebuildconfiguration.h"
#include "pysideuicextracompiler.h"
#include "pythonconstants.h"
#include "pythonlanguageclient.h"
#include "pythonproject.h"
#include "pythonsettings.h"
#include "pythontr.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <extensionsystem/pluginmanager.h>

#include <languageclient/languageclientmanager.h>

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/textdocument.h>

#include <utils/aspects.h>
#include <utils/fileutils.h>
#include <utils/futuresynchronizer.h>
#include <utils/layoutbuilder.h>
#include <utils/outputformatter.h>
#include <utils/theme/theme.h>

#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

class PythonOutputLineParser : public OutputLineParser
{
public:
    PythonOutputLineParser()
        // Note that moc dislikes raw string literals.
        : filePattern("^(\\s*)(File \"([^\"]+)\", line (\\d+), .*$)")
    {
        TaskHub::clearTasks(PythonErrorTaskCategory);
    }

private:
    Result handleLine(const QString &text, OutputFormat format) final
    {
        if (!m_inTraceBack) {
            m_inTraceBack = format == StdErrFormat
                    && text.startsWith("Traceback (most recent call last):");
            if (m_inTraceBack)
                return Status::InProgress;
            return Status::NotHandled;
        }

        const Id category(PythonErrorTaskCategory);
        const QRegularExpressionMatch match = filePattern.match(text);
        if (match.hasMatch()) {
            const LinkSpec link(match.capturedStart(2), match.capturedLength(2), match.captured(2));
            const auto fileName = FilePath::fromString(match.captured(3));
            const int lineNumber = match.captured(4).toInt();
            m_tasks.append({Task::Warning, QString(), fileName, lineNumber, category});
            return {Status::InProgress, {link}};
        }

        Status status = Status::InProgress;
        if (text.startsWith(' ')) {
            // Neither traceback start, nor file, nor error message line.
            // Not sure if that can actually happen.
            if (m_tasks.isEmpty()) {
                m_tasks.append({Task::Warning, text.trimmed(), {}, -1, category});
            } else {
                Task &task = m_tasks.back();
                if (!task.summary.isEmpty())
                    task.summary += ' ';
                task.summary += text.trimmed();
            }
        } else {
            // The actual exception. This ends the traceback.
            TaskHub::addTask({Task::Error, text, {}, -1, category});
            for (auto rit = m_tasks.crbegin(), rend = m_tasks.crend(); rit != rend; ++rit)
                TaskHub::addTask(*rit);
            m_tasks.clear();
            m_inTraceBack = false;
            status = Status::Done;
        }
        return status;
    }

    bool handleLink(const QString &href) final
    {
        const QRegularExpressionMatch match = filePattern.match(href);
        if (!match.hasMatch())
            return false;
        const QString fileName = match.captured(3);
        const int lineNumber = match.captured(4).toInt();
        Core::EditorManager::openEditorAt({FilePath::fromString(fileName), lineNumber});
        return true;
    }

    const QRegularExpression filePattern;
    QList<Task> m_tasks;
    bool m_inTraceBack;
};

////////////////////////////////////////////////////////////////

class PythonInterpreterAspectPrivate : public QObject
{
public:
    PythonInterpreterAspectPrivate(PythonInterpreterAspect *parent, RunConfiguration *rc)
        : q(parent), rc(rc)
    {
        connect(q, &InterpreterAspect::changed,
                this, &PythonInterpreterAspectPrivate::currentInterpreterChanged);

        connect(PySideInstaller::instance(), &PySideInstaller::pySideInstalled, this,
                [this](const FilePath &python) {
                    if (python == q->currentInterpreter().command)
                        checkForPySide(python);
                }
        );

        connect(rc->target(), &Target::buildSystemUpdated,
                this, &PythonInterpreterAspectPrivate::updateExtraCompilers);
    }

    ~PythonInterpreterAspectPrivate() { qDeleteAll(m_extraCompilers); }

    void checkForPySide(const FilePath &python);
    void checkForPySide(const FilePath &python, const QString &pySidePackageName);
    void handlePySidePackageInfo(const PipPackageInfo &pySideInfo,
                                 const FilePath &python,
                                 const QString &requestedPackageName);
    void updateExtraCompilers();
    void currentInterpreterChanged();

    struct PySideTools
    {
        FilePath pySideProjectPath;
        FilePath pySideUicPath;
    };
    void updateTools(const PySideTools &tools);

    FilePath m_pySideUicPath;

    PythonInterpreterAspect *q;
    RunConfiguration *rc;
    QList<PySideUicExtraCompiler *> m_extraCompilers;
    QFutureWatcher<PipPackageInfo> *m_watcher = nullptr;
    QMetaObject::Connection m_watcherConnection;
};

PythonInterpreterAspect::PythonInterpreterAspect(AspectContainer *container, RunConfiguration *rc)
    : InterpreterAspect(container), d(new PythonInterpreterAspectPrivate(this, rc))
{
    setSettingsKey("PythonEditor.RunConfiguation.Interpreter");
    setSettingsDialogId(Constants::C_PYTHONOPTIONS_PAGE_ID);

    updateInterpreters(PythonSettings::interpreters());

    const QList<Interpreter> interpreters = PythonSettings::detectPythonVenvs(
        rc->project()->projectDirectory());
    Interpreter defaultInterpreter = interpreters.isEmpty() ? PythonSettings::defaultInterpreter()
                                                            : interpreters.first();
    if (!defaultInterpreter.command.isExecutableFile())
        defaultInterpreter = PythonSettings::interpreters().value(0);
    if (defaultInterpreter.command.isExecutableFile()) {
        const IDeviceConstPtr device = DeviceKitAspect::device(rc->kit());
        if (device && !device->handlesFile(defaultInterpreter.command)) {
            defaultInterpreter = Utils::findOr(PythonSettings::interpreters(),
                                               defaultInterpreter,
                                               [device](const Interpreter &interpreter) {
                                                   return device->handlesFile(interpreter.command);
                                               });
        }
    }
    setDefaultInterpreter(defaultInterpreter);

    connect(PythonSettings::instance(), &PythonSettings::interpretersChanged,
            this, &InterpreterAspect::updateInterpreters);
}

PythonInterpreterAspect::~PythonInterpreterAspect()
{
    delete d;
}

void PythonInterpreterAspectPrivate::checkForPySide(const FilePath &python)
{
    PySideTools tools;
    const FilePath dir = python.parentDir();
    tools.pySideProjectPath = dir.pathAppended("pyside6-project").withExecutableSuffix();
    tools.pySideUicPath = dir.pathAppended("pyside6-uic").withExecutableSuffix();

    if (tools.pySideProjectPath.isExecutableFile() && tools.pySideUicPath.isExecutableFile())
        updateTools(tools);
    else
        checkForPySide(python, "PySide6-Essentials");
}

void PythonInterpreterAspectPrivate::checkForPySide(const FilePath &python,
                                                    const QString &pySidePackageName)
{
    const PipPackage package(pySidePackageName);
    QObject::disconnect(m_watcherConnection);
    delete m_watcher;
    m_watcher = new QFutureWatcher<PipPackageInfo>(this);
    m_watcherConnection = QObject::connect(m_watcher, &QFutureWatcherBase::finished, q, [=] {
        handlePySidePackageInfo(m_watcher->result(), python, pySidePackageName);
    });
    const auto future = Pip::instance(python)->info(package);
    m_watcher->setFuture(future);
    ExtensionSystem::PluginManager::futureSynchronizer()->addFuture(future);
}

void PythonInterpreterAspectPrivate::handlePySidePackageInfo(const PipPackageInfo &pySideInfo,
                                                             const FilePath &python,
                                                             const QString &requestedPackageName)
{
    const auto findPythonTools = [](const FilePaths &files,
                                    const FilePath &location,
                                    const FilePath &python) -> PySideTools {
        PySideTools result;
        const QString pySide6ProjectName
            = OsSpecificAspects::withExecutableSuffix(python.osType(), "pyside6-project");
        const QString pySide6UicName
            = OsSpecificAspects::withExecutableSuffix(python.osType(), "pyside6-uic");
        for (const FilePath &file : files) {
            if (file.fileName() == pySide6ProjectName) {
                result.pySideProjectPath = python.withNewMappedPath(location.resolvePath(file));
                result.pySideProjectPath = result.pySideProjectPath.cleanPath();
                if (!result.pySideUicPath.isEmpty())
                    return result;
            } else if (file.fileName() == pySide6UicName) {
                result.pySideUicPath = python.withNewMappedPath(location.resolvePath(file));
                result.pySideUicPath = result.pySideUicPath.cleanPath();
                if (!result.pySideProjectPath.isEmpty())
                    return result;
            }
        }
        return {};
    };

    PySideTools tools = findPythonTools(pySideInfo.files, pySideInfo.location, python);
    if (!tools.pySideProjectPath.isExecutableFile() && requestedPackageName != "PySide6") {
        checkForPySide(python, "PySide6");
        return;
    }

    updateTools(tools);
}

void PythonInterpreterAspectPrivate::currentInterpreterChanged()
{
    const FilePath python = q->currentInterpreter().command;
    checkForPySide(python);

    for (FilePath &file : rc->project()->files(Project::AllFiles)) {
        if (auto document = TextEditor::TextDocument::textDocumentForFilePath(file)) {
            if (document->mimeType() == Constants::C_PY_MIMETYPE
                || document->mimeType() == Constants::C_PY3_MIMETYPE) {
                PyLSConfigureAssistant::openDocumentWithPython(python, document);
                PySideInstaller::checkPySideInstallation(python, document);
            }
        }
    }
}

void PythonInterpreterAspectPrivate::updateTools(const PySideTools &tools)
{
    m_pySideUicPath = tools.pySideUicPath;

    updateExtraCompilers();

    if (Target *target = rc->target()) {
        if (BuildConfiguration *buildConfiguration = target->activeBuildConfiguration()) {
            if (BuildStepList *buildSteps = buildConfiguration->buildSteps()) {
                if (auto buildStep = buildSteps->firstOfType<PySideBuildStep>())
                    buildStep->updatePySideProjectPath(tools.pySideProjectPath);
            }
        }
    }
}

QList<PySideUicExtraCompiler *> PythonInterpreterAspect::extraCompilers() const
{
    return d->m_extraCompilers;
}

void PythonInterpreterAspectPrivate::updateExtraCompilers()
{
    QList<PySideUicExtraCompiler *> oldCompilers = m_extraCompilers;
    m_extraCompilers.clear();

    if (m_pySideUicPath.isExecutableFile()) {
        auto uiMatcher = [](const Node *node) {
            if (const FileNode *fileNode = node->asFileNode())
                return fileNode->fileType() == FileType::Form;
            return false;
        };
        const FilePaths uiFiles = rc->project()->files(uiMatcher);
        for (const FilePath &uiFile : uiFiles) {
            FilePath generated = uiFile.parentDir();
            generated = generated.pathAppended("/ui_" + uiFile.baseName() + ".py");
            int index = Utils::indexOf(oldCompilers, [&](PySideUicExtraCompiler *oldCompiler) {
                return oldCompiler->pySideUicPath() == m_pySideUicPath
                       && oldCompiler->project() == rc->project() && oldCompiler->source() == uiFile
                       && oldCompiler->targets() == FilePaths{generated};
            });
            if (index < 0) {
                m_extraCompilers << new PySideUicExtraCompiler(m_pySideUicPath,
                                                               rc->project(),
                                                               uiFile,
                                                               {generated},
                                                               this);
            } else {
                m_extraCompilers << oldCompilers.takeAt(index);
            }
        }
    }
    for (LanguageClient::Client *client : LanguageClient::LanguageClientManager::clients()) {
        if (auto pylsClient = qobject_cast<PyLSClient *>(client))
            pylsClient->updateExtraCompilers(rc->project(), m_extraCompilers);
    }
    qDeleteAll(oldCompilers);
}

// RunConfiguration

class PythonRunConfiguration : public RunConfiguration
{
public:
    PythonRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        buffered.setSettingsKey("PythonEditor.RunConfiguation.Buffered");
        buffered.setLabelText(Tr::tr("Buffered output"));
        buffered.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
        buffered.setToolTip(Tr::tr("Enabling improves output performance, "
                                   "but results in delayed output."));

        mainScript.setSettingsKey("PythonEditor.RunConfiguation.Script");
        mainScript.setLabelText(Tr::tr("Script:"));
        mainScript.setReadOnly(true);

        environment.setSupportForBuildEnvironment(target);

        arguments.setMacroExpander(macroExpander());

        workingDir.setMacroExpander(macroExpander());

        x11Forwarding.setMacroExpander(macroExpander());
        x11Forwarding.setVisible(HostOsInfo::isAnyUnixHost());

        setCommandLineGetter([this] {
            CommandLine cmd{interpreter.currentInterpreter().command};
            if (!buffered())
                cmd.addArg("-u");
            cmd.addArg(mainScript().fileName());
            cmd.addArgs(arguments(), CommandLine::Raw);
            return cmd;
        });

        setUpdater([this] {
            const BuildTargetInfo bti = buildTargetInfo();
            setDefaultDisplayName(Tr::tr("Run %1").arg(bti.targetFilePath.toUserOutput()));
            mainScript.setValue(bti.targetFilePath);
            workingDir.setDefaultWorkingDirectory(bti.targetFilePath.parentDir());
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }

    PythonInterpreterAspect interpreter{this, this};
    BoolAspect buffered{this};
    MainScriptAspect mainScript{this};
    EnvironmentAspect environment{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
    X11ForwardingAspect x11Forwarding{this};
};

// Factories

PythonRunConfigurationFactory::PythonRunConfigurationFactory()
{
    registerRunConfiguration<PythonRunConfiguration>(Constants::C_PYTHONRUNCONFIGURATION_ID);
    addSupportedProjectType(PythonProjectId);
}

PythonOutputFormatterFactory::PythonOutputFormatterFactory()
{
    setFormatterCreator([](Target *t) -> QList<OutputLineParser *> {
        if (t && t->project()->mimeType() == Constants::C_PY_MIMETYPE)
            return {new PythonOutputLineParser};
        return {};
    });
}

} // Python::Internal
