// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectsettingswidget.h>

namespace ProjectExplorer {

class Project;

namespace Internal {

class CodeStyleSettingsWidget : public ProjectSettingsWidget
{
    Q_OBJECT
public:
    explicit CodeStyleSettingsWidget(Project *project);
};

} // namespace Internal
} // namespace ProjectExplorer
