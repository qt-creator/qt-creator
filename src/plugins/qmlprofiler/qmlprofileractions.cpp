// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofileractions.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilertool.h"
#include "qmlprofilertr.h"

#include <debugger/analyzer/analyzerconstants.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <QMenu>

namespace QmlProfiler {
namespace Internal {

using namespace Core;
using namespace Debugger::Constants;

QmlProfilerActions::QmlProfilerActions(QObject *parent) : QObject(parent)
{}

void QmlProfilerActions::attachToTool(QmlProfilerTool *tool)
{
    const QString description = Tr::tr("The QML Profiler can be used to find performance "
                                       "bottlenecks in applications using QML.");

    m_runAction = std::make_unique<QAction>(Tr::tr("QML Profiler"));
    m_runAction->setToolTip(description);
    QObject::connect(m_runAction.get(), &QAction::triggered,
                     tool, &QmlProfilerTool::profileStartupProject);

    QAction *toolStartAction = tool->startAction();
    QObject::connect(toolStartAction, &QAction::changed, this, [this, toolStartAction] {
        m_runAction->setEnabled(toolStartAction->isEnabled());
    });

    m_attachAction = std::make_unique<QAction>(Tr::tr("QML Profiler (Attach to Waiting Application)"));
    m_attachAction->setToolTip(description);
    QObject::connect(m_attachAction.get(), &QAction::triggered,
                     tool, &QmlProfilerTool::attachToWaitingApplication);

    m_loadQmlTrace = std::make_unique<QAction>(Tr::tr("Load QML Trace"));
    connect(m_loadQmlTrace.get(), &QAction::triggered,
            tool, &QmlProfilerTool::showLoadDialog, Qt::QueuedConnection);

    m_saveQmlTrace = std::make_unique<QAction>(Tr::tr("Save QML Trace"));
    connect(m_saveQmlTrace.get(), &QAction::triggered,
            tool, &QmlProfilerTool::showSaveDialog, Qt::QueuedConnection);

    QmlProfilerStateManager *stateManager = tool->stateManager();
    connect(stateManager, &QmlProfilerStateManager::serverRecordingChanged,
            this, [this](bool recording) {
        m_loadQmlTrace->setEnabled(!recording);
    });
    m_loadQmlTrace->setEnabled(!stateManager->serverRecording());

    QmlProfilerModelManager *modelManager = tool->modelManager();
    connect(modelManager, &QmlProfilerModelManager::traceChanged,
            this, [this, modelManager] {
        m_saveQmlTrace->setEnabled(!modelManager->isEmpty());
    });
    m_saveQmlTrace->setEnabled(!modelManager->isEmpty());
}

void QmlProfilerActions::registerActions()
{
    m_options.reset(ActionManager::createMenu("Analyzer.Menu.QMLOptions"));
    m_options->menu()->setTitle(Tr::tr("QML Profiler Options"));
    m_options->menu()->setEnabled(true);
    ActionContainer *menu = ActionManager::actionContainer(M_DEBUG_ANALYZER);

    menu->addAction(ActionManager::registerAction(m_runAction.get(),
                                                  "QmlProfiler.Internal"),
                    Debugger::Constants::G_ANALYZER_TOOLS);
    menu->addAction(ActionManager::registerAction(m_attachAction.get(),
                                                  "QmlProfiler.AttachToWaitingApplication"),
                    Debugger::Constants::G_ANALYZER_REMOTE_TOOLS);

    menu->addMenu(m_options.get(), G_ANALYZER_OPTIONS);
    m_options->addAction(ActionManager::registerAction(m_loadQmlTrace.get(),
                                                       Constants::QmlProfilerLoadActionId));
    m_options->addAction(ActionManager::registerAction(m_saveQmlTrace.get(),
                                                       Constants::QmlProfilerSaveActionId));
}

} // namespace Internal
} // namespace QmlProfiler
