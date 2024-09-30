// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectsettingswidget.h>

#include <QVBoxLayout>

namespace ProjectExplorer {
class Project;
}

namespace Coco::Internal {

class CocoProjectSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
    Q_OBJECT

public:
    explicit CocoProjectSettingsWidget(ProjectExplorer::Project *project);
    ~CocoProjectSettingsWidget();

private:
    QVBoxLayout *m_layout;
};

} // namespace Coco::Internal
