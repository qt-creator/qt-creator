// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildsettings.h"
#include "projectexplorer/buildconfiguration.h"
#include "settings/cocoinstallation.h"
#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

#include <QPointer>
#include <QPushButton>
#include <QWidget>

namespace ProjectExplorer {
class Project;
}

namespace Coco::Internal {

class CocoProjectWidget : public QWidget
{
    Q_OBJECT
public:
    enum ConfigurationState { configDone, configEdited, configRunning, configStopped };

    explicit CocoProjectWidget(
        ProjectExplorer::Project *project, const ProjectExplorer::BuildConfiguration &buildConfig);

protected:
    void showEvent(QShowEvent *event) override;

public slots:
    void buildSystemUpdated(ProjectExplorer::BuildSystem *bs);
    void configurationErrorOccurred(const QString &error);

private slots:
    void onCoverageGroupBoxClicked();

    void onSaveButtonClicked();
    void onRevertButtonClicked();
    void onExcludeFileButtonClicked();
    void onExcludeDirButtonClicked();
    void onTweaksButtonClicked();

    void onTextChanged();

private:
    void displayChanges();
    void reloadSettings();
    void addCocoOption(QString option);
    void setState(ConfigurationState state);
    void readSelectionDir();
    void writeSelectionDir(const QString &path);
    void setTweaksVisible(bool on);
    void setMessageLabel(const Utils::InfoLabel::InfoType type, const QString &text);
    void clearMessageLabel();

    Utils::TextDisplay m_configerrorLabel;
    Utils::BoolAspect m_coverageGroupBoxEnabled;
    Layouting::Group m_coverageGroupbox{};
    Utils::StringAspect m_optionEdit;
    Layouting::PushButton m_tweaksButton{};
    Utils::TextDisplay m_tweaksDescriptionLabel;
    Utils::StringAspect m_tweaksEdit;
    Utils::StringAspect m_fileNameLabel;
    Utils::TextDisplay m_messageLabel;
    QPushButton m_revertButton;
    QPushButton m_saveButton;
    Utils::StringAspect m_changesText;

    ProjectExplorer::Project *m_project;
    QPointer<BuildSettings> m_buildSettings;
    QString m_selectionDirectory;
    ConfigurationState m_configState = configDone;
    QString m_buildConfigurationName;
    CocoInstallation m_coco;
};

} // namespace Coco::Internal
