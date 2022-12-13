// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "axivionprojectsettings.h"

#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"

#include <projectexplorer/project.h>
#include <utils/infolabel.h>

#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace Axivion::Internal {

const char PSK_PROJECTNAME[] = "Axivion.ProjectName";

AxivionProjectSettings::AxivionProjectSettings(ProjectExplorer::Project *project)
    : m_project{project}
{
    load();
    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &AxivionProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings,
            this, &AxivionProjectSettings::save);
}

void AxivionProjectSettings::load()
{
    m_dashboardProjectName = m_project->namedSettings(PSK_PROJECTNAME).toString();
}

void AxivionProjectSettings::save()
{
    m_project->setNamedSettings(PSK_PROJECTNAME, m_dashboardProjectName);
}

AxivionProjectSettingsWidget::AxivionProjectSettingsWidget(ProjectExplorer::Project *project,
                                                           QWidget *parent)
    : ProjectExplorer::ProjectSettingsWidget{parent}
    , m_projectSettings(AxivionPlugin::projectSettings(project))
    , m_globalSettings(AxivionPlugin::settings())
{
    setUseGlobalSettingsCheckBoxVisible(false);
    setUseGlobalSettingsLabelVisible(true);
    setGlobalSettingsId("Axivion.Settings.General"); // FIXME move id to constants
    // setup ui
    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    m_linkedProject = new QLabel(this);
    verticalLayout->addWidget(m_linkedProject);

    m_dashboardProjects = new QTreeWidget(this);
    m_dashboardProjects->setHeaderHidden(true);
    m_dashboardProjects->setRootIsDecorated(false);
    verticalLayout->addWidget(new QLabel(Tr::tr("Dashboard projects:")));
    verticalLayout->addWidget(m_dashboardProjects);

    m_infoLabel = new Utils::InfoLabel(this);
    m_infoLabel->setVisible(false);
    verticalLayout->addWidget(m_infoLabel);

    auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    m_fetchProjects = new QPushButton(Tr::tr("Fetch Projects"));
    horizontalLayout->addWidget(m_fetchProjects);
    m_link = new QPushButton(Tr::tr("Link Project"));
    m_link->setEnabled(false);
    horizontalLayout->addWidget(m_link);
    m_unlink = new QPushButton(Tr::tr("Unlink Project"));
    m_unlink->setEnabled(false);
    horizontalLayout->addWidget(m_unlink);
    verticalLayout->addLayout(horizontalLayout);

    connect(m_dashboardProjects, &QTreeWidget::itemSelectionChanged,
            this, &AxivionProjectSettingsWidget::updateEnabledStates);
    connect(m_fetchProjects, &QPushButton::clicked,
            this, &AxivionProjectSettingsWidget::fetchProjects);

    updateUi();
}

void AxivionProjectSettingsWidget::fetchProjects()
{
    m_dashboardProjects->clear();
    m_fetchProjects->setEnabled(false);
    // TODO perform query and populate m_dashboardProjects
}

void AxivionProjectSettingsWidget::updateUi()
{
    const QString projectName = m_projectSettings->dashboardProjectName();
    if (projectName.isEmpty())
        m_linkedProject->setText(Tr::tr("This project is not linked to a dashboard project."));
    else
        m_linkedProject->setText(Tr::tr("This project is linked to \"%1\".").arg(projectName));
    updateEnabledStates();
}

void AxivionProjectSettingsWidget::updateEnabledStates()
{
    const bool hasDashboardSettings = m_globalSettings->curl.isExecutableFile()
            && !m_globalSettings->server.dashboard.isEmpty()
            && !m_globalSettings->server.token.isEmpty();
    const bool linked = !m_projectSettings->dashboardProjectName().isEmpty();
    const bool linkable = m_dashboardProjects->topLevelItemCount()
            && !m_dashboardProjects->selectedItems().isEmpty();

    m_fetchProjects->setEnabled(hasDashboardSettings);
    m_link->setEnabled(!linked && linkable);
    m_unlink->setEnabled(linked);

    if (!hasDashboardSettings) {
        m_infoLabel->setText(Tr::tr("Incomplete or misconfigured settings."));
        m_infoLabel->setType(Utils::InfoLabel::NotOk);
        m_infoLabel->setVisible(true);
    }
}

} // Axivion::Internal
