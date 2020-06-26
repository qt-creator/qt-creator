/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "pythonrunconfiguration.h"

#include "pythonconstants.h"
#include "pythonproject.h"
#include "pythonsettings.h"
#include "pythonutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <languageclient/languageclientmanager.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/textdocument.h>

#include <utils/fileutils.h>
#include <utils/outputformatter.h>
#include <utils/theme/theme.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QFormLayout>
#include <QPlainTextEdit>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python {
namespace Internal {

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

        const Utils::Id category(PythonErrorTaskCategory);
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
        Core::EditorManager::openEditorAt(fileName, lineNumber);
        return true;
    }

    const QRegularExpression filePattern;
    QList<Task> m_tasks;
    bool m_inTraceBack;
};

////////////////////////////////////////////////////////////////

class InterpreterAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    InterpreterAspect() = default;

    Interpreter currentInterpreter() const;
    void updateInterpreters(const QList<Interpreter> &interpreters);
    void setDefaultInterpreter(const Interpreter &interpreter) { m_defaultId = interpreter.id; }

    void fromMap(const QVariantMap &) override;
    void toMap(QVariantMap &) const override;
    void addToLayout(LayoutBuilder &builder) override;

private:
    void updateCurrentInterpreter();
    void updateComboBox();
    QList<Interpreter> m_interpreters;
    QPointer<QComboBox> m_comboBox;
    QString m_defaultId;
    QString m_currentId;
};

Interpreter InterpreterAspect::currentInterpreter() const
{
    return m_comboBox ? m_interpreters.value(m_comboBox->currentIndex()) : Interpreter();
}

void InterpreterAspect::updateInterpreters(const QList<Interpreter> &interpreters)
{
    m_interpreters = interpreters;
    if (m_comboBox)
        updateComboBox();
}

void InterpreterAspect::fromMap(const QVariantMap &map)
{
    m_currentId = map.value(settingsKey(), m_defaultId).toString();
}

void InterpreterAspect::toMap(QVariantMap &map) const
{
    map.insert(settingsKey(), m_currentId);
}

void InterpreterAspect::addToLayout(LayoutBuilder &builder)
{
    if (QTC_GUARD(m_comboBox.isNull()))
        m_comboBox = new QComboBox;

    updateComboBox();
    connect(m_comboBox, &QComboBox::currentTextChanged,
            this, &InterpreterAspect::updateCurrentInterpreter);

    auto manageButton = new QPushButton(tr("Manage..."));
    connect(manageButton, &QPushButton::clicked, []() {
        Core::ICore::showOptionsDialog(Constants::C_PYTHONOPTIONS_PAGE_ID);
    });

    builder.addItems(tr("Interpreter"), m_comboBox.data(), manageButton);
}

void InterpreterAspect::updateCurrentInterpreter()
{
    m_currentId = currentInterpreter().id;
    m_comboBox->setToolTip(currentInterpreter().command.toUserOutput());
    emit changed();
}

void InterpreterAspect::updateComboBox()
{
    int currentIndex = -1;
    int defaultIndex = -1;
    const QString currentId = m_currentId;
    m_comboBox->clear();
    for (const Interpreter &interpreter : m_interpreters) {
        int index = m_comboBox->count();
        m_comboBox->addItem(interpreter.name);
        m_comboBox->setItemData(index, interpreter.command.toUserOutput(), Qt::ToolTipRole);
        if (interpreter.id == currentId)
            currentIndex = index;
        if (interpreter.id == m_defaultId)
            defaultIndex = index;
    }
    if (currentIndex >= 0)
        m_comboBox->setCurrentIndex(currentIndex);
    else if (defaultIndex >= 0)
        m_comboBox->setCurrentIndex(defaultIndex);
    updateCurrentInterpreter();
}

class MainScriptAspect : public BaseStringAspect
{
    Q_OBJECT

public:
    MainScriptAspect() = default;
};

PythonRunConfiguration::PythonRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration(target, id)
{
    auto interpreterAspect = addAspect<InterpreterAspect>();
    interpreterAspect->setSettingsKey("PythonEditor.RunConfiguation.Interpreter");
    connect(interpreterAspect, &InterpreterAspect::changed,
            this, &PythonRunConfiguration::updateLanguageServer);

    connect(PythonSettings::instance(), &PythonSettings::interpretersChanged,
            interpreterAspect, &InterpreterAspect::updateInterpreters);

    QList<Interpreter> interpreters = PythonSettings::detectPythonVenvs(project()->projectDirectory());
    aspect<InterpreterAspect>()->updateInterpreters(PythonSettings::interpreters());
    aspect<InterpreterAspect>()->setDefaultInterpreter(
        interpreters.isEmpty() ? PythonSettings::defaultInterpreter() : interpreters.first());

    auto bufferedAspect = addAspect<BaseBoolAspect>();
    bufferedAspect->setSettingsKey("PythonEditor.RunConfiguation.Buffered");
    bufferedAspect->setLabel(tr("Buffered output"), BaseBoolAspect::LabelPlacement::AtCheckBox);
    bufferedAspect->setToolTip(tr("Enabling improves output performance, "
                                  "but results in delayed output."));

    auto scriptAspect = addAspect<MainScriptAspect>();
    scriptAspect->setSettingsKey("PythonEditor.RunConfiguation.Script");
    scriptAspect->setLabelText(tr("Script:"));
    scriptAspect->setDisplayStyle(BaseStringAspect::LabelDisplay);

    addAspect<LocalEnvironmentAspect>(target);

    auto argumentsAspect = addAspect<ArgumentsAspect>();

    addAspect<WorkingDirectoryAspect>();
    addAspect<TerminalAspect>();

    setCommandLineGetter([this, bufferedAspect, interpreterAspect, argumentsAspect] {
        CommandLine cmd{interpreterAspect->currentInterpreter().command};
        if (!bufferedAspect->value())
            cmd.addArg("-u");
        cmd.addArg(mainScript());
        cmd.addArgs(argumentsAspect->arguments(macroExpander()), CommandLine::Raw);
        return cmd;
    });

    setUpdater([this, scriptAspect] {
        const BuildTargetInfo bti = buildTargetInfo();
        const QString script = bti.targetFilePath.toUserOutput();
        setDefaultDisplayName(tr("Run %1").arg(script));
        scriptAspect->setValue(script);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

void PythonRunConfiguration::updateLanguageServer()
{
    using namespace LanguageClient;

    const FilePath python(FilePath::fromUserInput(interpreter()));

    for (FilePath &file : project()->files(Project::AllFiles)) {
        if (auto document = TextEditor::TextDocument::textDocumentForFilePath(file)) {
            if (document->mimeType() == Constants::C_PY_MIMETYPE)
                PyLSConfigureAssistant::instance()->openDocumentWithPython(python, document);
        }
    }

    aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(
        Utils::FilePath::fromString(mainScript()).parentDir());
}

bool PythonRunConfiguration::supportsDebugger() const
{
    return true;
}

QString PythonRunConfiguration::mainScript() const
{
    return aspect<MainScriptAspect>()->value();
}

QString PythonRunConfiguration::arguments() const
{
    return aspect<ArgumentsAspect>()->arguments(macroExpander());
}

QString PythonRunConfiguration::interpreter() const
{
    return aspect<InterpreterAspect>()->currentInterpreter().command.toString();
}

PythonRunConfigurationFactory::PythonRunConfigurationFactory()
{
    registerRunConfiguration<PythonRunConfiguration>("PythonEditor.RunConfiguration.");
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

} // namespace Internal
} // namespace Python

#include "pythonrunconfiguration.moc"
