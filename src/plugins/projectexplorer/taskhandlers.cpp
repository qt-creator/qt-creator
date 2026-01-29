// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "taskhandlers.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "taskhub.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/customlanguagemodels.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/vcsmanager.h>
#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QHash>
#include <QTimer>

#include <memory>
#include <vector>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace {
static QPointer<QObject> g_actionParent;
static Core::Context g_cmdContext;
static Internal::RegisterHandlerAction g_onCreateAction;
static Internal::GetHandlerTasks g_getTasks;
static QList<ITaskHandler *> g_toRegister;
static QList<ITaskHandler *> g_taskHandlers;
} // namespace

ITaskHandler::ITaskHandler(QAction *action, const Id &actionId, bool isMultiHandler)
    : m_action(action), m_actionId(actionId), m_isMultiHandler(isMultiHandler)
{
    if (g_actionParent)
        registerHandler();
    else
        g_toRegister << this;
}

ITaskHandler::~ITaskHandler()
{
    g_toRegister.removeOne(this);
    deregisterHandler();
    delete m_action;
}

void ITaskHandler::handle(const Task &task)
{
    QTC_ASSERT(m_isMultiHandler, return);
    handle(Tasks{task});
}

void ITaskHandler::handle(const Tasks &tasks)
{
    QTC_ASSERT(canHandle(tasks), return);
    QTC_ASSERT(!m_isMultiHandler, return);
    handle(tasks.first());
}

bool ITaskHandler::canHandle(const Tasks &tasks) const
{
    if (tasks.isEmpty())
        return false;
    if (m_isMultiHandler)
        return true;
    if (tasks.size() > 1)
        return false;
    return canHandle(tasks.first());
}

void ITaskHandler::registerHandler()
{
    g_taskHandlers.append(this);

    m_action->setParent(g_actionParent);
    QAction *action = m_action;
    connect(m_action, &QAction::triggered, this, [this] { handle(g_getTasks()); });
    if (m_actionId.isValid()) {
        Core::Command *cmd =
            Core::ActionManager::registerAction(m_action, m_actionId, g_cmdContext, true);
        action = cmd->action();
    }
    g_onCreateAction(action);
}

void ITaskHandler::deregisterHandler()
{
    g_taskHandlers.removeOne(this);
}

namespace Internal {
namespace {

class ConfigTaskHandler : public ITaskHandler
{
public:
    ConfigTaskHandler(const Task &pattern, Id page)
        : ITaskHandler(createAction())
        , m_pattern(pattern)
        , m_targetPage(page)
    {}

private:
    bool canHandle(const Task &task) const override
    {
        return task.description() == m_pattern.description()
               && task.category() == m_pattern.category();
    }

    void handle(const Task &task) override
    {
        Q_UNUSED(task)
        ICore::showSettings(m_targetPage);
    }

    QAction *createAction() const
    {
        auto action = new QAction(ICore::msgShowSettings());
        action->setToolTip(ICore::msgShowSettingsToolTip());
        return action;
    }

private:
    const Task m_pattern;
    const Id m_targetPage;
};

class CopyTaskHandler : public ITaskHandler
{
public:
    CopyTaskHandler() : ITaskHandler(new Action, Core::Constants::COPY, true) {}

private:
    void handle(const Tasks &tasks) override
    {
        QStringList lines;
        for (const Task &task : tasks) {
            QString type;
            switch (task.type()) {
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
            lines << task.file().toUserOutput() + ':' + QString::number(task.line())
                         + ": " + type + task.description();
        }
        setClipboardAndSelection(lines.join('\n'));
    }
};

class RemoveTaskHandler : public ITaskHandler
{
public:
    RemoveTaskHandler() : ITaskHandler(createAction(), {}, true) {}

private:
    void handle(const Tasks &tasks) override
    {
        for (const Task &task : tasks)
            TaskHub::removeTask(task);
    }

    QAction *createAction() const
    {
        QAction *removeAction = new QAction(
            Tr::tr("Remove", "Name of the action triggering the removetaskhandler"));
        removeAction->setToolTip(Tr::tr("Remove task from the task list."));
        removeAction->setShortcuts({QKeySequence::Delete, QKeySequence::Backspace});
        removeAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        return removeAction;
    }
};

class ShowInEditorTaskHandler : public ITaskHandler
{
public:
    ShowInEditorTaskHandler() : ITaskHandler(createAction()) {}

private:
    bool isDefaultHandler() const override { return true; }

    bool canHandle(const Task &task) const override
    {
        return task.file().isReadableFile();
    }

    void handle(const Task &task) override
    {
        const int column = task.column() ? task.column() - 1 : 0;
        EditorManager::openEditorAt(
            {task.file(), task.line(), column}, {}, EditorManager::SwitchSplitIfAlreadyVisible);
    }

    QAction *createAction() const
    {
        QAction *showAction = new QAction(Tr::tr("Show in Editor"));
        showAction->setToolTip(Tr::tr("Show task location in an editor."));
        showAction->setShortcut(QKeySequence(Qt::Key_Return));
        showAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        return showAction;
    }
};

class VcsAnnotateTaskHandler : public ITaskHandler
{
public:
    VcsAnnotateTaskHandler() : ITaskHandler(createAction()) {}

private:
    bool canHandle(const Task &task) const override
    {
        if (!task.file().isReadableFile())
            return false;
        IVersionControl *vc = VcsManager::findVersionControlForDirectory(task.file().absolutePath());
        if (!vc)
            return false;
        return vc->supportsOperation(IVersionControl::AnnotateOperation);
    }

    void handle(const Task &task) override
    {
        IVersionControl *vc = VcsManager::findVersionControlForDirectory(task.file().absolutePath());
        QTC_ASSERT(vc, return);
        QTC_ASSERT(vc->supportsOperation(IVersionControl::AnnotateOperation), return);
        vc->vcsAnnotate(task.file().absoluteFilePath(), task.line());
    }

    QAction *createAction() const
    {
        QAction *vcsannotateAction = new QAction(Tr::tr("&Annotate"));
        vcsannotateAction->setToolTip(Tr::tr("Annotate using version control system."));
        return vcsannotateAction;
    }
};

class ExplainWithAiHandler : public ITaskHandler
{
public:
    ExplainWithAiHandler(const QString &model)
        : ITaskHandler(createAction(model))
        , m_model(model)
    {}

private:
    bool canHandle(const Task &task) const override
    {
        Q_UNUSED(task)
        return !availableLanguageModels().isEmpty();
    }

    void handle(const Task &task) override
    {
        const CommandLine cmdLine = commandLineForLanguageModel(m_model);
        QTC_ASSERT(!cmdLine.isEmpty(), return);

        QString prompt;
        if (task.origin().isEmpty())
            prompt += "A software tool has emitted a diagnostic. ";
        else
            prompt += QString("The tool \"%1\" has emitted a diagnostic. ").arg(task.origin());
        prompt += "Please explain what it is about. "
                  "Be as concise and concrete as possible and try to name the root cause."
                  "If you don't know the answer, just say so."
                  "If possible, also provide a solution. "
                  "Do not think for more than a minute. "
                  "Here is the error: ###%1##";
        prompt = prompt.arg(task.description());
        if (task.file().exists()) {
            if (const auto contents = task.file().fileContents()) {
                prompt.append('\n').append(
                    "Ideally, provide your solution in the form of a diff."
                    "Here are the contents of the file that the tool complained about: ###%1###."
                    "The path to the file is ###%2###.");
                prompt = prompt.arg(QString::fromUtf8(*contents), task.file().toUserOutput());
            }
        }
        const auto process = new Process(this);
        process->setCommand(cmdLine);
        process->setProcessMode(ProcessMode::Writer);
        process->setTextChannelMode(Channel::Output, TextChannelMode::MultiLine);
        process->setTextChannelMode(Channel::Error, TextChannelMode::MultiLine);
        connect(process, &Process::textOnStandardOutput,
                [](const QString &text) { MessageManager::writeFlashing(text); });
        connect(process, &Process::done, [process] {
            MessageManager::writeSilently(
                process->exitMessage(Process::FailureMessageFormat::WithStdErr));
            process->deleteLater();
        });
        connect(process, &Process::started, [process, prompt] {
            process->write(prompt);
            process->closeWriteChannel();
        });
        QTimer::singleShot(60000, process, [process] { process->kill(); });
        MessageManager::writeDisrupting(Tr::tr("Querying %1...").arg(m_model));
        process->start();
    }

    QAction *createAction(const QString &model) const
    {
        const auto action = new QAction(Tr::tr("Get help from %1").arg(model));
        action->setToolTip(Tr::tr("Ask the %1 LLM to help with this issue.").arg(model));
        return action;
    }

    const QString m_model;
};

class AiHandlersManager
{
public:
    AiHandlersManager()
    {
        QObject::connect(&customLanguageModelsContext(), &BaseAspect::changed,
                         g_actionParent, [this] { reset(); });
        reset();
    }

private:
    void reset()
    {
        m_aiTaskHandlers.clear();
        for (const QString &model : availableLanguageModels())
            m_aiTaskHandlers.emplace_back(std::make_unique<ExplainWithAiHandler>(model));
        updateTaskHandlerActionsState();
    }

    std::vector<std::unique_ptr<ExplainWithAiHandler>> m_aiTaskHandlers;
};

} // namespace

void setupTaskHandlers(
    QObject *actionParent,
    const Core::Context &cmdContext,
    const RegisterHandlerAction &onCreateAction,
    const GetHandlerTasks &getTasks)
{
    g_actionParent = actionParent;
    g_cmdContext = cmdContext;
    g_onCreateAction = onCreateAction;
    g_getTasks = getTasks;

    static const ConfigTaskHandler
        configTaskHandler(Task::compilerMissingTask(), Constants::KITS_SETTINGS_PAGE_ID);
    static const CopyTaskHandler copyTaskHandler;
    static const RemoveTaskHandler removeTaskHandler;
    static const ShowInEditorTaskHandler showInEditorTaskHandler;
    static const VcsAnnotateTaskHandler vcsAnnotateTaskHandler;
    static const AiHandlersManager aiHandlersManager;

    registerQueuedTaskHandlers();
}

ITaskHandler *taskHandlerForAction(QAction *action)
{
    return Utils::findOrDefault(g_taskHandlers,
                                [action](ITaskHandler *h) { return h->action() == action; });
}

void updateTaskHandlerActionsState()
{
    const Tasks tasks = g_getTasks();
    for (ITaskHandler * const h : g_taskHandlers)
        h->action()->setEnabled(h->canHandle(tasks));
}

ITaskHandler *defaultTaskHandler()
{
    return Utils::findOrDefault(g_taskHandlers,
                                [](ITaskHandler *h) { return h->isDefaultHandler(); });
}

void registerQueuedTaskHandlers()
{
    for (ITaskHandler * const h : std::as_const(g_toRegister))
        h->registerHandler();
    g_toRegister.clear();
}

} // namespace Internal
} // namespace ProjectExplorer
