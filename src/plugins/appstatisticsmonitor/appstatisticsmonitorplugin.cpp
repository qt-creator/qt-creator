// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "manager.h"

#include <extensionsystem/iplugin.h>

#include <QString>

namespace AppStatisticsMonitor::Internal {

class AppStatisticsMonitorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AppStatisticsMonitor.json")

private:
    void initialize() final;
    std::unique_ptr<AppStatisticsMonitorManager> m_appstatisticsmonitorManager;
    std::unique_ptr<AppStatisticsMonitorViewFactory> m_appstatisticsmonitorViewFactory;
};

void AppStatisticsMonitorPlugin::initialize()
{
    m_appstatisticsmonitorManager = std::make_unique<AppStatisticsMonitorManager>();
    m_appstatisticsmonitorViewFactory = std::make_unique<AppStatisticsMonitorViewFactory>(
        m_appstatisticsmonitorManager.get());
}

} // namespace AppStatisticsMonitor::Internal

#include "appstatisticsmonitorplugin.moc"
