/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilerplugin.h"
#include "qmlprofilerruncontrolfactory.h"

#include "qmlprofilertool.h"
#include "qmlprofilertimelinemodel.h"

#include <analyzerbase/analyzermanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QOpenGLContext>
#include <QtPlugin>

using namespace Analyzer;

namespace QmlProfiler {
namespace Internal {

bool QmlProfilerPlugin::debugOutput = false;
QmlProfilerPlugin *QmlProfilerPlugin::instance = 0;

bool QmlProfilerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)

    if (!QOpenGLContext().create()) {
        *errorString = tr("Cannot create OpenGL context.");
        return false;
    }

    auto tool = new QmlProfilerTool(this);
    auto widgetCreator = [tool] { return tool->createWidgets(); };
    auto runControlCreator = [tool](const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration) {
        return tool->createRunControl(sp, runConfiguration);
    };

    AnalyzerAction *action = 0;

    QString description = QmlProfilerTool::tr(
        "The QML Profiler can be used to find performance bottlenecks in "
        "applications using QML.");

    action = new AnalyzerAction(this);
    action->setActionId(QmlProfilerLocalActionId);
    action->setToolId(QmlProfilerToolId);
    action->setWidgetCreator(widgetCreator);
    action->setRunControlCreator(runControlCreator);
    action->setToolPreparer([tool] { return tool->prepareTool(); });
    action->setRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    action->setText(tr("QML Profiler"));
    action->setToolTip(description);
    action->setMenuGroup(Constants::G_ANALYZER_TOOLS);
    AnalyzerManager::addAction(action);

    action = new AnalyzerAction(this);
    action->setActionId(QmlProfilerRemoteActionId);
    action->setToolId(QmlProfilerToolId);
    action->setWidgetCreator(widgetCreator);
    action->setRunControlCreator(runControlCreator);
    action->setCustomToolStarter([tool] { tool->startRemoteTool(); });
    action->setToolPreparer([tool] { return tool->prepareTool(); });
    action->setRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    action->setText(tr("QML Profiler (External)"));
    action->setToolTip(description);
    action->setMenuGroup(Constants::G_ANALYZER_REMOTE_TOOLS);
    AnalyzerManager::addAction(action);

    addAutoReleasedObject(new QmlProfilerRunControlFactory());
    QmlProfilerPlugin::instance = this;

    return true;
}

void QmlProfilerPlugin::extensionsInitialized()
{
    factory = ExtensionSystem::PluginManager::getObject<QmlProfilerTimelineModelFactory>();
}

ExtensionSystem::IPlugin::ShutdownFlag QmlProfilerPlugin::aboutToShutdown()
{
    // Save settings.
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

QList<QmlProfilerTimelineModel *> QmlProfilerPlugin::getModels(QmlProfilerModelManager *manager) const
{
    if (factory)
        return factory->create(manager);
    else
        return QList<QmlProfilerTimelineModel *>();
}

} // namespace Internal
} // namespace QmlProfiler
