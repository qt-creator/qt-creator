// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonrunconfiguration.h"

#include "pythonconstants.h"
#include "pythonproject.h"
#include "pythontr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/aspects.h>
#include <utils/fileutils.h>
#include <utils/outputformatter.h>

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

        x11Forwarding.setVisible(HostOsInfo::isAnyUnixHost());

        interpreter.setLabelText(Tr::tr("Python:"));
        interpreter.setReadOnly(true);

        setCommandLineGetter([this] {
            CommandLine cmd;
            cmd.setExecutable(interpreter());
            if (interpreter().isEmpty())
                return cmd;
            if (!buffered())
                cmd.addArg("-u");
            cmd.addArg(mainScript().fileName());
            cmd.addArgs(arguments(), CommandLine::Raw);
            return cmd;
        });

        setUpdater([this] {
            const BuildTargetInfo bti = buildTargetInfo();
            const auto python = FilePath::fromSettings(bti.additionalData.toMap().value("python"));
            interpreter.setValue(python);

            setDefaultDisplayName(Tr::tr("Run %1").arg(bti.targetFilePath.toUserOutput()));
            mainScript.setValue(bti.targetFilePath);
            workingDir.setDefaultWorkingDirectory(bti.targetFilePath.parentDir());
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }

    FilePathAspect interpreter{this};
    BoolAspect buffered{this};
    MainScriptAspect mainScript{this};
    EnvironmentAspect environment{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
    X11ForwardingAspect x11Forwarding{this};
};

// Factories

class PythonRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    PythonRunConfigurationFactory()
    {
        registerRunConfiguration<PythonRunConfiguration>(Constants::C_PYTHONRUNCONFIGURATION_ID);
        addSupportedProjectType(PythonProjectId);
    }
};

void setupPythonRunConfiguration()
{
    static PythonRunConfigurationFactory thePythonRunConfigurationFactory;
}

void setupPythonRunWorker()
{
    static ProcessRunnerFactory thePythonRunWorkerFactory(
        {Constants::C_PYTHONRUNCONFIGURATION_ID}
    );
}

void setupPythonDebugWorker()
{
    static Debugger::SimpleDebugRunnerFactory thePythonDebugRunWorkerFactory(
        {Constants::C_PYTHONRUNCONFIGURATION_ID},
        {ProjectExplorer::Constants::DAP_PY_DEBUG_RUN_MODE}
    );
}

void setupPythonOutputParser()
{
    addOutputParserFactory([](Target *t) -> OutputLineParser * {
        if (t && t->project()->mimeType() == Constants::C_PY_PROJECT_MIME_TYPE)
            return new PythonOutputLineParser;
        return nullptr;
    });
}

} // Python::Internal
