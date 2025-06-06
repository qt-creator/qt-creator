// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "taskhandlers.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "taskhub.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <utils/stringutils.h>

using namespace Core;

namespace ProjectExplorer::Internal {
namespace {

class ConfigTaskHandler : public ITaskHandler
{
public:
    ConfigTaskHandler(const Task &pattern, Utils::Id page)
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
    const Utils::Id m_targetPage;
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
        Utils::setClipboardAndSelection(lines.join('\n'));
    }

    Utils::Id actionManagerId() const override { return Utils::Id(Core::Constants::COPY); }
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

} // namespace

void setupTaskHandlers()
{
    static const ConfigTaskHandler
        configTaskHandler(Task::compilerMissingTask(), Constants::KITS_SETTINGS_PAGE_ID);
    static const CopyTaskHandler copyTaskHandler;
    static const RemoveTaskHandler removeTaskHandler;
    static const ShowInEditorTaskHandler showInEditorTaskHandler;
    static const VcsAnnotateTaskHandler vcsAnnotateTaskHandler;
}

} // namespace ProjectExplorer::Internal
