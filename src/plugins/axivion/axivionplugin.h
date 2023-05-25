// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace ProjectExplorer { class Project; }

namespace Axivion::Internal {

class AxivionSettings;
class AxivionProjectSettings;
class ProjectInfo;

class AxivionPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Axivion.json")

public:
    AxivionPlugin();
    ~AxivionPlugin() final;

    static AxivionPlugin *instance();
    static AxivionSettings *settings();
    static AxivionProjectSettings *projectSettings(ProjectExplorer::Project *project);

    static bool handleCertificateIssue();
    static void fetchProjectInfo(const QString &projectName);
    static ProjectInfo projectInfo();
signals:
    void settingsChanged();

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final {}
};

} // Axivion::Internal

