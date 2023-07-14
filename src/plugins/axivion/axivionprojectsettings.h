// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "axivionsettings.h"

#include <projectexplorer/projectsettingswidget.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QTreeWidget;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace Utils { class InfoLabel; }

namespace Axivion::Internal {

class DashboardInfo;

class AxivionProjectSettings : public QObject
{
public:
    explicit AxivionProjectSettings(ProjectExplorer::Project *project);

    void setDashboardProjectName(const QString &name) { m_dashboardProjectName = name; }
    QString dashboardProjectName() const { return m_dashboardProjectName; }

private:
    void load();
    void save();

    ProjectExplorer::Project *m_project = nullptr;
    QString m_dashboardProjectName;
};

class AxivionProjectSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
public:
    explicit AxivionProjectSettingsWidget(ProjectExplorer::Project *project,
                                          QWidget *parent = nullptr);

private:
    void fetchProjects();
    void onDashboardInfoReceived(const DashboardInfo &info);
    void onSettingsChanged();
    void linkProject();
    void unlinkProject();
    void updateUi();
    void updateEnabledStates();

    AxivionProjectSettings *m_projectSettings = nullptr;
    QLabel *m_linkedProject = nullptr;
    QTreeWidget *m_dashboardProjects = nullptr;
    QPushButton *m_fetchProjects = nullptr;
    QPushButton *m_link = nullptr;
    QPushButton *m_unlink = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;
};

} // Axivion::Internal
