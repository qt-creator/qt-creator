// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectsettingswidget.h"

#include <texteditor/commentssettings.h>

namespace ProjectExplorer {
class Project;

namespace Internal {

class ProjectCommentsSettings
{
public:
    // Passing a null ptr is allowed and yields the global settings, so you can use
    // this class transparently for both cases.
    ProjectCommentsSettings(Project *project);

    TextEditor::CommentsSettings::Data settings() const;
    void setSettings(const TextEditor::CommentsSettings::Data &settings);
    bool useGlobalSettings() const { return m_useGlobalSettings; }
    void setUseGlobalSettings(bool useGlobal);

private:
    void loadSettings();
    void saveSettings();

    ProjectExplorer::Project * const m_project;
    TextEditor::CommentsSettings::Data m_customSettings;
    bool m_useGlobalSettings = true;
};

class ProjectCommentsSettingsWidget : public ProjectSettingsWidget
{
public:
    ProjectCommentsSettingsWidget(ProjectExplorer::Project *project);
    ~ProjectCommentsSettingsWidget();

private:
    class Private;
    Private * const d;
};

} // namespace Internal
} // namespace ProjectExplorer

