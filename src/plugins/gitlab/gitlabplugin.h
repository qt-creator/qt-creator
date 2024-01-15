// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace ProjectExplorer { class Project; }
namespace Utils { class Id; }

namespace GitLab {

class GitLabProjectSettings;

GitLabProjectSettings *projectSettings(ProjectExplorer::Project *project);

bool handleCertificateIssue(const Utils::Id &serverId);

void linkedStateChanged(bool enabled);

} // GitLab
