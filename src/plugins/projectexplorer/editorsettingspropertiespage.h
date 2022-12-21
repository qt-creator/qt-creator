// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectsettingswidget.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QGroupBox;
class QPushButton;
class QSpinBox;
QT_END_NAMESPACE

namespace TextEditor { class BehaviorSettingsWidget; }

namespace ProjectExplorer {

class EditorConfiguration;
class Project;

namespace Internal {

class EditorSettingsWidget : public ProjectSettingsWidget
{
    Q_OBJECT
public:
    explicit EditorSettingsWidget(Project *project);

private:
    void globalSettingsActivated(bool useGlobal);
    void restoreDefaultValues();

    void settingsToUi(const EditorConfiguration *config);

    Project *m_project;

    QPushButton *m_restoreButton;
    QCheckBox *m_showWrapColumn;
    QCheckBox *m_tintMarginArea;
    QSpinBox *m_wrapColumn;
    QCheckBox *m_useIndenter;
    QGroupBox *m_displaySettings;
    TextEditor::BehaviorSettingsWidget *m_behaviorSettings;
};

} // namespace Internal
} // namespace ProjectExplorer
