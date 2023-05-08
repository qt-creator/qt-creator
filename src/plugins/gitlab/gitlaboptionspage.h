// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gitlabparameters.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace GitLab {

namespace Constants { const char GITLAB_SETTINGS[] = "GitLab"; }

class GitLabOptionsPage : public Core::IOptionsPage
{
public:
    explicit GitLabOptionsPage(GitLabParameters *p);
};

} // GitLab
