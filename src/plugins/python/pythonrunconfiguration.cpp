// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonrunconfiguration.h"

#include "pyside.h"
#include "pythonbuildconfiguration.h"
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

#include <QUrl>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

static const QRegularExpression &tracebackFilePattern()
{
    static const QRegularExpression s_filePattern("^(\\s*)(File \"([^\"]+)\", line (\\d+), .*$)");
    return s_filePattern;
}

static const QRegularExpression &moduleNotFoundPattern()
{
    static const QRegularExpression s_functionPattern(
        "^ModuleNotFoundError: No module named '([_a-zA-Z][_a-zA-Z0-9]*)'$");
    return s_functionPattern;
}

class PythonOutputLineParser : public OutputLineParser
{
public:
    PythonOutputLineParser(const FilePath &python)
        : m_python(python)
    {
        TaskHub::clearTasks(PythonErrorTaskCategory);
    }

private:
    Result handleLine(const QString &text, OutputFormat format) final
    {
        const Id category(PythonErrorTaskCategory);

        if (m_inTraceBack) {
            const QRegularExpressionMatch match = tracebackFilePattern().match(text);
            if (match.hasMatch()) {
                const LinkSpec link(match.capturedStart(2), match.capturedLength(2), match.captured(2));
                const auto fileName = FilePath::fromUserInput(match.captured(3));
                const int lineNumber = match.captured(4).toInt();
                m_tasks.append({Task::Warning, QString(), fileName, lineNumber, category});
                return {Status::InProgress, {link}};
            }

            if (text.startsWith(' ')) {
                // Neither traceback start, nor file, nor error message line.
                // Not sure if that can actually happen.
                if (m_tasks.isEmpty()) {
                    m_tasks.append({Task::Warning, text.trimmed(), {}, -1, category});
                } else {
                    Task &task = m_tasks.back();
                    if (!task.summary().isEmpty())
                        task.addToSummary(QChar(' '));
                    task.addToSummary(text.trimmed());
                }
            } else {
                // The actual exception. This ends the traceback.
                Task exception{Task::Error, text, {}, -1, category};
                const QString detail = Tr::tr("Install %1 (requires pip)");
                const QString pySide6Text = Tr::tr("PySide6");
                const QString link = QString("pysideinstall:")
                                     + QUrl::toPercentEncoding(m_python.toFSPathString());
                exception.addToDetails(detail.arg(pySide6Text));
                QTextCharFormat format;
                format.setAnchor(true);
                format.setAnchorHref(link);
                const int offset = exception.summary().length() + detail.indexOf("%1") + 1;
                exception.setFormats(
                    {QTextLayout::FormatRange{offset, int(pySide6Text.length()), format}});
                TaskHub::addTask(exception);
                for (auto rit = m_tasks.crbegin(), rend = m_tasks.crend(); rit != rend; ++rit)
                    TaskHub::addTask(*rit);
                m_inTraceBack = false;
                const QRegularExpressionMatch match = moduleNotFoundPattern().match(text);
                if (match.hasMatch()) {
                    const QString moduleName = match.captured(1);
                    if (moduleName == "PySide6") {
                        const LinkSpec
                            link(match.capturedStart(1), match.capturedLength(1), moduleName);
                        return {Status::Done, {link}};
                    }
                }
                return Status::Done;
            }
            return Status::InProgress;
        }

        if (format == StdErrFormat) {
            m_inTraceBack = text.startsWith("Traceback (most recent call last):");
            if (m_inTraceBack)
                return Status::InProgress;
        }

        return Status::NotHandled;
    }

    bool handleLink(const QString &href) final
    {
        if (const QRegularExpressionMatch match = tracebackFilePattern().match(href);
            match.hasMatch()) {
            const QString fileName = match.captured(3);
            const int lineNumber = match.captured(4).toInt();
            Core::EditorManager::openEditorAt({FilePath::fromString(fileName), lineNumber});
            return true;
        }
        if (href == "PySide6")
            PySideInstaller::instance().installPySide(m_python, href);
        return false;
    }

    QList<Task> m_tasks;
    bool m_inTraceBack = false;
    const FilePath m_python;
};

// RunConfiguration

class PythonRunConfiguration : public RunConfiguration
{
public:
    PythonRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        buffered.setSettingsKey("PythonEditor.RunConfiguation.Buffered");
        buffered.setLabelText(Tr::tr("Buffered output"));
        buffered.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
        buffered.setToolTip(Tr::tr("Enabling improves output performance, "
                                   "but results in delayed output."));

        mainScript.setSettingsKey("PythonEditor.RunConfiguation.Script");
        mainScript.setLabelText(Tr::tr("Script:"));
        mainScript.setReadOnly(true);

        environment.setSupportForBuildEnvironment(bc);

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
    addOutputParserFactory([](BuildConfiguration *bc) -> OutputLineParser * {
        auto *pythonBuildConfig = qobject_cast<PythonBuildConfiguration *>(bc);
        if (!pythonBuildConfig)
            return nullptr;

        Target *t = pythonBuildConfig->target();
        QTC_ASSERT(t, return nullptr);

        if (t->project()->mimeType() == Constants::C_PY_PROJECT_MIME_TYPE
            || t->project()->mimeType() == Constants::C_PY_PROJECT_MIME_TYPE_TOML) {
            return new PythonOutputLineParser(pythonBuildConfig->python());
        }
        return nullptr;
    });
}

} // namespace Python::Internal
