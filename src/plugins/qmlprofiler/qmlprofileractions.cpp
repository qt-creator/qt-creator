/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qmlprofileractions.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilertool.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilermodelmanager.h"

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
    const QString description = tr("The QML Profiler can be used to find performance "
                                   "bottlenecks in applications using QML.");

    m_runAction = std::make_unique<QAction>(tr("QML Profiler"));
    m_runAction->setToolTip(description);
    QObject::connect(m_runAction.get(), &QAction::triggered,
                     tool, &QmlProfilerTool::profileStartupProject);

    QAction *toolStartAction = tool->startAction();
    QObject::connect(toolStartAction, &QAction::changed, this, [this, toolStartAction] {
        m_runAction->setEnabled(toolStartAction->isEnabled());
    });

    m_attachAction = std::make_unique<QAction>(tr("QML Profiler (Attach to Waiting Application)"));
    m_attachAction->setToolTip(description);
    QObject::connect(m_attachAction.get(), &QAction::triggered,
                     tool, &QmlProfilerTool::attachToWaitingApplication);

    m_loadQmlTrace = std::make_unique<QAction>(tr("Load QML Trace"));
    connect(m_loadQmlTrace.get(), &QAction::triggered,
            tool, &QmlProfilerTool::showLoadDialog, Qt::QueuedConnection);

    m_saveQmlTrace = std::make_unique<QAction>(tr("Save QML Trace"));
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
    m_options->menu()->setTitle(tr("QML Profiler Options"));
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
