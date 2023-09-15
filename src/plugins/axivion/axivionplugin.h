// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dashboard/dashboardclient.h"

#include <extensionsystem/iplugin.h>

#include <memory>

namespace ProjectExplorer { class Project; }

namespace Axivion::Internal {

class AxivionProjectSettings;
class ProjectInfo;

class AxivionPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Axivion.json")

public:
    AxivionPlugin() {}
    ~AxivionPlugin() final;

    static void fetchProjectInfo(const QString &projectName);
    static std::shared_ptr<const DashboardClient::ProjectInfo> projectInfo();
    static bool handleCertificateIssue();

private:
    void initialize() final;
    void extensionsInitialized() final {}
};

} // Axivion::Internal

