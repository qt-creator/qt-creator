// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gitlabparameters.h"

#include <extensionsystem/iplugin.h>

namespace ProjectExplorer { class Project; }

namespace GitLab {

class Events;
class GitLabProjectSettings;

class GitLabPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GitLab.json")

public:
    GitLabPlugin();
    ~GitLabPlugin() override;

    void initialize() override;

    static GitLabProjectSettings *projectSettings(ProjectExplorer::Project *project);
    static bool handleCertificateIssue(const Utils::Id &serverId);

    static void linkedStateChanged(bool enabled);
private:
    void openView();
    void onStartupProjectChanged();
};

} // namespace GitLab
