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

#include <languageclient/languageclientmanager.h>

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/textdocument.h>

#include <utils/aspects.h>
#include <utils/fileutils.h>
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

class PythonRunConfigurationPrivate
{
public:
    PythonRunConfigurationPrivate(PythonRunConfiguration *rc)
        : q(rc)
    {}
    ~PythonRunConfigurationPrivate()
    {
        qDeleteAll(m_extraCompilers);
    }

    void checkForPySide(const Utils::FilePath &python);
    void checkForPySide(const Utils::FilePath &python, const QString &pySidePackageName);
    void handlePySidePackageInfo(const PipPackageInfo &pySideInfo,
                                 const Utils::FilePath &python,
                                 const QString &requestedPackageName);
    void updateExtraCompilers();
    Utils::FilePath m_pySideUicPath;

    PythonRunConfiguration *q;
    QList<PySideUicExtraCompiler *> m_extraCompilers;
    QFutureWatcher<PipPackageInfo> m_watcher;
    QMetaObject::Connection m_watcherConnection;

};

PythonRunConfiguration::PythonRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
    , d(new PythonRunConfigurationPrivate(this))
{
    auto interpreterAspect = addAspect<InterpreterAspect>();
    interpreterAspect->setSettingsKey("PythonEditor.RunConfiguation.Interpreter");
    interpreterAspect->setSettingsDialogId(Constants::C_PYTHONOPTIONS_PAGE_ID);

    connect(interpreterAspect, &InterpreterAspect::changed,
            this, &PythonRunConfiguration::currentInterpreterChanged);

    connect(PythonSettings::instance(), &PythonSettings::interpretersChanged,
            interpreterAspect, &InterpreterAspect::updateInterpreters);

    const QList<Interpreter> interpreters = PythonSettings::detectPythonVenvs(
        project()->projectDirectory());
    interpreterAspect->updateInterpreters(PythonSettings::interpreters());
    Interpreter defaultInterpreter = interpreters.isEmpty() ? PythonSettings::defaultInterpreter()
                                                            : interpreters.first();
    if (!defaultInterpreter.command.isExecutableFile())
        defaultInterpreter = PythonSettings::interpreters().value(0);
    if (defaultInterpreter.command.isExecutableFile()) {
        const IDeviceConstPtr device = DeviceKitAspect::device(target->kit());
        if (device && !device->handlesFile(defaultInterpreter.command)) {
            defaultInterpreter = Utils::findOr(PythonSettings::interpreters(),
                                               defaultInterpreter,
                                               [device](const Interpreter &interpreter) {
                                                   return device->handlesFile(interpreter.command);
                                               });
        }
    }
    interpreterAspect->setDefaultInterpreter(defaultInterpreter);

    auto bufferedAspect = addAspect<BoolAspect>();
    bufferedAspect->setSettingsKey("PythonEditor.RunConfiguation.Buffered");
    bufferedAspect->setLabel(Tr::tr("Buffered output"), BoolAspect::LabelPlacement::AtCheckBox);
    bufferedAspect->setToolTip(Tr::tr("Enabling improves output performance, "
                                  "but results in delayed output."));

    auto scriptAspect = addAspect<MainScriptAspect>();
    scriptAspect->setSettingsKey("PythonEditor.RunConfiguation.Script");
    scriptAspect->setLabelText(Tr::tr("Script:"));
    scriptAspect->setDisplayStyle(StringAspect::LabelDisplay);

    addAspect<LocalEnvironmentAspect>(target);

    auto argumentsAspect = addAspect<ArgumentsAspect>(macroExpander());

    addAspect<WorkingDirectoryAspect>(macroExpander(), nullptr);
    addAspect<TerminalAspect>();

    setCommandLineGetter([bufferedAspect, interpreterAspect, argumentsAspect, scriptAspect] {
        CommandLine cmd{interpreterAspect->currentInterpreter().command};
        if (!bufferedAspect->value())
            cmd.addArg("-u");
        cmd.addArg(scriptAspect->filePath().fileName());
        cmd.addArgs(argumentsAspect->arguments(), CommandLine::Raw);
        return cmd;
    });

    setUpdater([this, scriptAspect] {
        const BuildTargetInfo bti = buildTargetInfo();
        const QString script = bti.targetFilePath.toUserOutput();
        setDefaultDisplayName(Tr::tr("Run %1").arg(script));
        scriptAspect->setValue(script);
        aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.targetFilePath.parentDir());
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    connect(target, &Target::buildSystemUpdated, this, [this]() { d->updateExtraCompilers(); });
    currentInterpreterChanged();

    setRunnableModifier([](Runnable &r) {
        r.workingDirectory = r.command.executable().withNewMappedPath(r.workingDirectory); // FIXME: Needed?
    });

    connect(PySideInstaller::instance(), &PySideInstaller::pySideInstalled, this,
            [this](const FilePath &python) {
                if (python == aspect<InterpreterAspect>()->currentInterpreter().command)
                    d->checkForPySide(python);
            });
}

PythonRunConfiguration::~PythonRunConfiguration()
{
    delete d;
}

void PythonRunConfigurationPrivate::checkForPySide(const FilePath &python)
{
    checkForPySide(python, "PySide6-Essentials");
}

void PythonRunConfigurationPrivate::checkForPySide(const FilePath &python,
                                                   const QString &pySidePackageName)
{
    const PipPackage package(pySidePackageName);
    QObject::disconnect(m_watcherConnection);
    m_watcherConnection = QObject::connect(&m_watcher,
                                           &QFutureWatcher<PipPackageInfo>::finished,
                                           q,
                                           [=]() {
                                               handlePySidePackageInfo(m_watcher.result(),
                                                                       python,
                                                                       pySidePackageName);
                                           });
    m_watcher.setFuture(Pip::instance(python)->info(package));
}

void PythonRunConfigurationPrivate::handlePySidePackageInfo(const PipPackageInfo &pySideInfo,
                                                            const Utils::FilePath &python,
                                                            const QString &requestedPackageName)
{
    struct PythonTools
    {
        FilePath pySideProjectPath;
        FilePath pySideUicPath;
    };

    BuildStepList *buildSteps = nullptr;
    if (Target *target = q->target()) {
        if (auto buildConfiguration = target->activeBuildConfiguration())
            buildSteps = buildConfiguration->buildSteps();
    }
    if (!buildSteps)
        return;

    const auto findPythonTools = [](const FilePaths &files,
                                    const FilePath &location,
                                    const FilePath &python) -> PythonTools {
        PythonTools result;
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

    PythonTools pythonTools = findPythonTools(pySideInfo.files, pySideInfo.location, python);
    if (!pythonTools.pySideProjectPath.isExecutableFile() && requestedPackageName != "PySide6") {
        checkForPySide(python, "PySide6");
        return;
    }

    m_pySideUicPath = pythonTools.pySideUicPath;

    updateExtraCompilers();

    if (auto pySideBuildStep = buildSteps->firstOfType<PySideBuildStep>())
        pySideBuildStep->updatePySideProjectPath(pythonTools.pySideProjectPath);
}

void PythonRunConfiguration::currentInterpreterChanged()
{
    const FilePath python = aspect<InterpreterAspect>()->currentInterpreter().command;
    d->checkForPySide(python);

    for (FilePath &file : project()->files(Project::AllFiles)) {
        if (auto document = TextEditor::TextDocument::textDocumentForFilePath(file)) {
            if (document->mimeType() == Constants::C_PY_MIMETYPE
                || document->mimeType() == Constants::C_PY3_MIMETYPE) {
                PyLSConfigureAssistant::openDocumentWithPython(python, document);
                PySideInstaller::checkPySideInstallation(python, document);
            }
        }
    }
}

QList<PySideUicExtraCompiler *> PythonRunConfiguration::extraCompilers() const
{
    return d->m_extraCompilers;
}

void PythonRunConfigurationPrivate::updateExtraCompilers()
{
    QList<PySideUicExtraCompiler *> oldCompilers = m_extraCompilers;
    m_extraCompilers.clear();

    if (m_pySideUicPath.isExecutableFile()) {
        auto uiMatcher = [](const ProjectExplorer::Node *node) {
            if (const ProjectExplorer::FileNode *fileNode = node->asFileNode())
                return fileNode->fileType() == ProjectExplorer::FileType::Form;
            return false;
        };
        const FilePaths uiFiles = q->project()->files(uiMatcher);
        for (const FilePath &uiFile : uiFiles) {
            FilePath generated = uiFile.parentDir();
            generated = generated.pathAppended("/ui_" + uiFile.baseName() + ".py");
            int index = Utils::indexOf(oldCompilers, [&](PySideUicExtraCompiler *oldCompiler) {
                return oldCompiler->pySideUicPath() == m_pySideUicPath
                       && oldCompiler->project() == q->project() && oldCompiler->source() == uiFile
                       && oldCompiler->targets() == FilePaths{generated};
            });
            if (index < 0) {
                m_extraCompilers << new PySideUicExtraCompiler(m_pySideUicPath,
                                                               q->project(),
                                                               uiFile,
                                                               {generated},
                                                               q);
            } else {
                m_extraCompilers << oldCompilers.takeAt(index);
            }
        }
    }
    for (LanguageClient::Client *client : LanguageClient::LanguageClientManager::clients()) {
        if (auto pylsClient = qobject_cast<PyLSClient *>(client))
            pylsClient->updateExtraCompilers(q->project(), m_extraCompilers);
    }
    qDeleteAll(oldCompilers);
}

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
