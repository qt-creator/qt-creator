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

#ifndef QMLPROFILERPLUGIN_H
#define QMLPROFILERPLUGIN_H

#include "qmlprofiler_global.h"
#include "qmlprofilertimelinemodelfactory.h"
#include "qmlprofilersettings.h"
#include <extensionsystem/iplugin.h>

#include "qmlprofilertimelinemodel.h"

namespace QmlProfiler {
namespace Internal {

class QmlProfilerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlProfiler.json")

public:
    QmlProfilerPlugin() : factory(0) {}

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    static bool debugOutput;
    static QmlProfilerPlugin *instance;

    QList<QmlProfilerTimelineModel *> getModels(QmlProfilerModelManager *manager) const;
    static QmlProfilerSettings *globalSettings();

private:
    QmlProfilerTimelineModelFactory *factory;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERPLUGIN_H

