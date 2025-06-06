// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "taskhandlers.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "taskhub.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/customlanguagemodels.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/vcsmanager.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QTimer>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer::Internal {
namespace {

class ConfigTaskHandler : public ITaskHandler
{
public:
    ConfigTaskHandler(const Task &pattern, Id page)
        : m_pattern(pattern)
        , m_targetPage(page)
    {}

private:
    bool canHandle(const Task &task) const override
    {
        return task.description() == m_pattern.description() && task.category == m_pattern.category;
    }

    void handle(const Task &task) override
    {
        Q_UNUSED(task)
        ICore::showOptionsDialog(m_targetPage);
    }

    QAction *createAction(QObject *parent) const override
    {
        auto action = new QAction(ICore::msgShowOptionsDialog(), parent);
        action->setToolTip(ICore::msgShowOptionsDialogToolTip());
        return action;
    }

private:
    const Task m_pattern;
    const Id m_targetPage;
};

class CopyTaskHandler : public ITaskHandler
{
public:
    CopyTaskHandler() : ITaskHandler(true) {}

private:
    void handle(const Tasks &tasks) override
    {
        QStringList lines;
        for (const Task &task : tasks) {
            QString type;
            switch (task.type) {
            case Task::Error:
                //: Task is of type: error
                type = Tr::tr("error:") + QLatin1Char(' ');
                break;
            case Task::Warning:
                //: Task is of type: warning
                type = Tr::tr("warning:") + QLatin1Char(' ');
                break;
            default:
                break;
            }
            lines << task.file.toUserOutput() + ':' + QString::number(task.line)
                         + ": " + type + task.description();
        }
        setClipboardAndSelection(lines.join('\n'));
    }

    Id actionManagerId() const override { return Id(Core::Constants::COPY); }
    QAction *createAction(QObject *parent) const override { return new QAction(parent); }
};

class RemoveTaskHandler : public ITaskHandler
{
public:
    RemoveTaskHandler() : ITaskHandler(true) {}

private:
    void handle(const Tasks &tasks) override
    {
        for (const Task &task : tasks)
            TaskHub::removeTask(task);
    }

    QAction *createAction(QObject *parent) const override
    {
        QAction *removeAction = new QAction(
            Tr::tr("Remove", "Name of the action triggering the removetaskhandler"), parent);
        removeAction->setToolTip(Tr::tr("Remove task from the task list."));
        removeAction->setShortcuts({QKeySequence::Delete, QKeySequence::Backspace});
        removeAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        return removeAction;
    }
};

class ShowInEditorTaskHandler : public ITaskHandler
{
    bool isDefaultHandler() const override { return true; }

    bool canHandle(const Task &task) const override
    {
        if (task.file.isEmpty())
            return false;
        QFileInfo fi(task.file.toFileInfo());
        return fi.exists() && fi.isFile() && fi.isReadable();
    }

    void handle(const Task &task) override
    {
        const int column = task.column ? task.column - 1 : 0;
        EditorManager::openEditorAt(
            {task.file, task.movedLine, column}, {}, EditorManager::SwitchSplitIfAlreadyVisible);
    }

    QAction *createAction(QObject *parent ) const override
    {
        QAction *showAction = new QAction(Tr::tr("Show in Editor"), parent);
        showAction->setToolTip(Tr::tr("Show task location in an editor."));
        showAction->setShortcut(QKeySequence(Qt::Key_Return));
        showAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        return showAction;
    }
};

class VcsAnnotateTaskHandler : public ITaskHandler
{
    bool canHandle(const Task &task) const override
    {
        QFileInfo fi(task.file.toFileInfo());
        if (!fi.exists() || !fi.isFile() || !fi.isReadable())
            return false;
        IVersionControl *vc = VcsManager::findVersionControlForDirectory(task.file.absolutePath());
        if (!vc)
            return false;
        return vc->supportsOperation(IVersionControl::AnnotateOperation);
    }

    void handle(const Task &task) override
    {
        IVersionControl *vc = VcsManager::findVersionControlForDirectory(task.file.absolutePath());
        QTC_ASSERT(vc, return);
        QTC_ASSERT(vc->supportsOperation(IVersionControl::AnnotateOperation), return);
        vc->vcsAnnotate(task.file.absoluteFilePath(), task.movedLine);
    }

    QAction *createAction(QObject *parent) const override
    {
        QAction *vcsannotateAction = new QAction(Tr::tr("&Annotate"), parent);
        vcsannotateAction->setToolTip(Tr::tr("Annotate using version control system."));
        return vcsannotateAction;
    }
};

// FIXME: There should be one handler per LLM, but the ITaskHandler infrastructure is
// currently static. Alternatively, we could somehow multiplex from here, perhaps via a submenu.
class ExplainWithAiHandler : public ITaskHandler
{
    bool canHandle(const Task &task) const override
    {
        Q_UNUSED(task)
        return !availableLanguageModels().isEmpty();
    }

    void handle(const Task &task) override
    {
        const QStringList llms = availableLanguageModels();
        QTC_ASSERT(!llms.isEmpty(), return);

        QString prompt;
        if (task.origin.isEmpty())
            prompt += "A software tool has emitted a diagnostic. ";
        else
            prompt += QString("The tool \"%1\" has emitted a diagnostic. ").arg(task.origin);
        prompt += "Please explain what it is about. "
                  "Be as concise and concrete as possible and try to name the root cause."
                  "If you don't know the answer, just say so."
                  "If possible, also provide a solution. "
                  "Do not think for more than a minute. "
                  "Here is the error: ###%1##";
        prompt = prompt.arg(task.description());
        if (task.file.exists()) {
            if (const auto contents = task.file.fileContents()) {
                prompt.append('\n').append(
                    "Ideally, provide your solution in the form of a diff."
                    "Here are the contents of the file that the tool complained about: ###%1###."
                    "The path to the file is ###%2###.");
                prompt = prompt.arg(QString::fromUtf8(*contents), task.file.toUserOutput());
            }
        }
        const auto process = new Process;
        process->setCommand(commandLineForLanguageModel(llms.first()));
        process->setProcessMode(ProcessMode::Writer);
        process->setTextChannelMode(Channel::Output, TextChannelMode::MultiLine);
        process->setTextChannelMode(Channel::Error, TextChannelMode::MultiLine);
        connect(process, &Process::textOnStandardOutput,
                [](const QString &text) { MessageManager::writeFlashing(text); });
        connect(process, &Process::textOnStandardError,
                [](const QString &text) { MessageManager::writeFlashing(text); });
        connect(process, &Process::done, [process] {
            MessageManager::writeSilently(process->exitMessage());
            process->deleteLater();
        });
        connect(process, &Process::started, [process, prompt] {
            process->write(prompt);
            process->closeWriteChannel();
        });
        QTimer::singleShot(60000, process, [process] { process->kill(); });
        MessageManager::writeDisrupting(Tr::tr("Querying LLM..."));
        process->start();
    }

    QAction *createAction(QObject *parent) const override
    {
        const auto action = new QAction(Tr::tr("Get help from AI"), parent);
        action->setToolTip(Tr::tr("Ask an AI to help with this issue."));
        return action;
    }
};

} // namespace

void setupTaskHandlers()
{
    static const ConfigTaskHandler
        configTaskHandler(Task::compilerMissingTask(), Constants::KITS_SETTINGS_PAGE_ID);
    static const CopyTaskHandler copyTaskHandler;
    static const RemoveTaskHandler removeTaskHandler;
    static const ShowInEditorTaskHandler showInEditorTaskHandler;
    static const VcsAnnotateTaskHandler vcsAnnotateTaskHandler;
    static const ExplainWithAiHandler explainWithAiHandler;
}

} // namespace ProjectExplorer::Internal
