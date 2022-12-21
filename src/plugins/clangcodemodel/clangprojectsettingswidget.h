// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_clangprojectsettingswidget.h"

#include "clangprojectsettings.h"
#include <projectexplorer/projectsettingswidget.h>

#include <QPointer>

namespace ProjectExplorer { class Project; }

namespace ClangCodeModel {
namespace Internal {

class ClangProjectSettingsWidget: public ProjectExplorer::ProjectSettingsWidget
{
    Q_OBJECT

public:
    explicit ClangProjectSettingsWidget(ProjectExplorer::Project *project);

private:
    void onDelayedTemplateParseClicked(bool);
    void onGlobalCustomChanged(bool useGlobalSettings);
    void onAboutToSaveProjectSettings();

    void syncWidgets();
    void syncOtherWidgetsToComboBox();

private:
    Ui::ClangProjectSettingsWidget m_ui;
    ClangProjectSettings &m_projectSettings;
};

} // namespace Internal
} // namespace ClangCodeModel
