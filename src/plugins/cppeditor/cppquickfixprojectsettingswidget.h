// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppquickfixprojectsettings.h"
#include <projectexplorer/projectsettingswidget.h>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace CppEditor::Internal {

class CppQuickFixSettingsWidget;
class CppQuickFixProjectSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
    Q_OBJECT

public:
    explicit CppQuickFixProjectSettingsWidget(ProjectExplorer::Project *project,
                                              QWidget *parent = nullptr);
    ~CppQuickFixProjectSettingsWidget();

private:
    void currentItemChanged(bool useGlobalSettings);
    void buttonCustomClicked();

    CppQuickFixSettingsWidget *m_settingsWidget;
    CppQuickFixProjectsSettings::CppQuickFixProjectsSettingsPtr m_projectSettings;

    QPushButton *m_pushButton;
};

} // CppEditor::Internal
