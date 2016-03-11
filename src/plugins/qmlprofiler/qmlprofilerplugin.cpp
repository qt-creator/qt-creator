/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprofilerplugin.h"
#include "qmlprofilerruncontrolfactory.h"
#include "qmlprofileroptionspage.h"
#include "qmlprofilertool.h"
#include "qmlprofilertimelinemodel.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/hostosinfo.h>

#include <QtPlugin>

namespace QmlProfiler {
namespace Internal {

Q_GLOBAL_STATIC(QmlProfilerSettings, qmlProfilerGlobalSettings)

bool QmlProfilerPlugin::debugOutput = false;
QmlProfilerPlugin *QmlProfilerPlugin::instance = 0;

bool QmlProfilerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)

    QmlProfilerPlugin::instance = this;

    if (!Utils::HostOsInfo::canCreateOpenGLContext(errorString))
        return false;

    return true;
}

void QmlProfilerPlugin::extensionsInitialized()
{
    factory = ExtensionSystem::PluginManager::getObject<QmlProfilerTimelineModelFactory>();

    (void) new QmlProfilerTool(this);

    addAutoReleasedObject(new QmlProfilerRunControlFactory());
    addAutoReleasedObject(new Internal::QmlProfilerOptionsPage());
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

QmlProfilerSettings *QmlProfilerPlugin::globalSettings()
{
    return qmlProfilerGlobalSettings();
}

} // namespace Internal
} // namespace QmlProfiler
