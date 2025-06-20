// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "task.h"

#include <utils/id.h>

#include <QAction>
#include <QObject>
#include <QPointer>
#include <QString>

#include <functional>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core { class Context; }

namespace ProjectExplorer {
namespace Internal { void registerQueuedTaskHandlers(); }

class PROJECTEXPLORER_EXPORT ITaskHandler : public QObject
{
public:
    explicit ITaskHandler(QAction *action, const Utils::Id &actionId = {},
                          bool isMultiHandler = false);
    ~ITaskHandler() override;

    virtual bool isDefaultHandler() const { return false; }
    virtual bool canHandle(const Task &) const { return m_isMultiHandler; }
    virtual void handle(const Task &task);       // Non-multi-handlers should implement this.
    virtual void handle(const Tasks &tasks); // Multi-handlers should implement this.

    bool canHandle(const Tasks &tasks) const;
    QAction *action() const { return m_action; }

private:
    friend void Internal::registerQueuedTaskHandlers();

    void registerHandler();
    void deregisterHandler();

    const QPointer<QAction> m_action;
    const Utils::Id m_actionId;
    const bool m_isMultiHandler;
};

namespace Internal {
using RegisterHandlerAction = std::function<void(QAction *)>;
using GetHandlerTasks = std::function<Tasks()>;
void setupTaskHandlers(
    QObject *actionParent,
    const Core::Context &cmdContext,
    const RegisterHandlerAction &onCreateAction,
    const GetHandlerTasks &getTasks);

ITaskHandler *taskHandlerForAction(QAction *action);
ITaskHandler *defaultTaskHandler();
void updateTaskHandlerActionsState();

} // namespace Internal
} // namespace ProjectExplorer
