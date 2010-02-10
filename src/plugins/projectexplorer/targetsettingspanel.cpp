/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "targetsettingspanel.h"

#include "addtargetdialog.h"
#include "buildsettingspropertiespage.h"
#include "project.h"
#include "projectwindow.h"
#include "runsettingspropertiespage.h"
#include "target.h"
#include "targetsettingswidget.h"

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

///
// TargetSettingsPanelFactory
///

QString TargetSettingsPanelFactory::id() const
{
    return QLatin1String(TARGETSETTINGS_PANEL_ID);
}

QString TargetSettingsPanelFactory::displayName() const
{
    return QCoreApplication::tr("Targets");
}

bool TargetSettingsPanelFactory::supports(Project *project)
{
    Q_UNUSED(project);
    return true;
}

bool TargetSettingsPanelFactory::supports(Target *target)
{
    Q_UNUSED(target);
    return false;
}

IPropertiesPanel *TargetSettingsPanelFactory::createPanel(Project *project)
{
    return new TargetSettingsPanel(project);
}

IPropertiesPanel *TargetSettingsPanelFactory::createPanel(Target *target)
{
    Q_UNUSED(target);
    return 0;
}

///
// TargetSettingsPanel
///

TargetSettingsPanel::TargetSettingsPanel(Project *project) :
    m_widget(new TargetSettingsPanelWidget(project))
{
}

TargetSettingsPanel::~TargetSettingsPanel()
{
    delete m_widget;
}

QString TargetSettingsPanel::displayName() const
{
    return QCoreApplication::tr("Targets");
}

QWidget *TargetSettingsPanel::widget() const
{
    return m_widget;
}

QIcon TargetSettingsPanel::icon() const
{
    return QIcon();
}

///
// TargetSettingsWidget
///

TargetSettingsPanelWidget::TargetSettingsPanelWidget(Project *project) :
    m_currentIndex(-1),
    m_project(project),
    m_selector(0),
    m_centralWidget(0)
{
    m_panelWidgets[0] = 0;
    m_panelWidgets[1] = 0;

    setupUi();

    connect(m_project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(targetAdded(ProjectExplorer::Target*)));
    connect(m_project, SIGNAL(aboutToRemoveTarget(ProjectExplorer::Target*)),
            this, SLOT(aboutToRemoveTarget(ProjectExplorer::Target*)));
    connect(m_project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(activeTargetChanged(ProjectExplorer::Target*)));
}

TargetSettingsPanelWidget::~TargetSettingsPanelWidget()
{
}

void TargetSettingsPanelWidget::setupUi()
{
    QVBoxLayout *viewLayout = new QVBoxLayout(this);
    viewLayout->setMargin(0);
    viewLayout->setSpacing(0);

    m_selector = new TargetSettingsWidget(this);
    viewLayout->addWidget(m_selector);

    // Setup our container for the contents:
    m_centralWidget = new QStackedWidget(this);
    m_selector->setCentralWidget(m_centralWidget);

    // no projects label:
    m_noTargetLabel = new QWidget;
    QVBoxLayout *noTargetLayout = new QVBoxLayout;
    noTargetLayout->setMargin(0);
    QLabel *label = new QLabel(m_noTargetLabel);
    label->setText(tr("No target defined."));
    {
        QFont f = label->font();
        f.setPointSizeF(f.pointSizeF() * 1.4);
        f.setBold(true);
        label->setFont(f);
    }
    label->setMargin(10);
    label->setAlignment(Qt::AlignTop);
    noTargetLayout->addWidget(label);
    noTargetLayout->addStretch(10);
    m_centralWidget->addWidget(m_noTargetLabel);

    foreach (Target *t, m_project->targets())
        targetAdded(t);
    m_selector->markActive(m_project->targets().indexOf(m_project->activeTarget()));

    connect(m_selector, SIGNAL(currentIndexChanged(int,int)),
            this, SLOT(currentTargetIndexChanged(int,int)));
    connect(m_selector, SIGNAL(addButtonClicked()),
            this, SLOT(addTarget()));
    connect(m_selector, SIGNAL(removeButtonClicked()),
            this, SLOT(removeTarget()));

    if (m_project->targets().count())
        currentTargetIndexChanged(m_project->targets().indexOf(m_project->activeTarget()), 0);
}

void TargetSettingsPanelWidget::currentTargetIndexChanged(int targetIndex, int subIndex)
{
    if (targetIndex < -1 || targetIndex >= m_project->targets().count())
        return;
    if (subIndex < -1 || subIndex >= 2)
        return;
    m_selector->setCurrentIndex(targetIndex);
    m_selector->setCurrentSubIndex(subIndex);

    Target *target(m_project->targets().at(targetIndex));

    // Target was not actually changed:
    if (m_currentIndex == targetIndex) {
        m_centralWidget->setCurrentWidget(m_panelWidgets[subIndex]);
        return;
    }

    // Target has changed:
    delete m_panelWidgets[0];
    m_panelWidgets[0] = 0;
    delete m_panelWidgets[1];
    m_panelWidgets[1] = 0;

    if (targetIndex == -1) { // no more targets!
        m_centralWidget->setCurrentWidget(m_noTargetLabel);
        return;
    }

    PanelsWidget *buildPanel(new PanelsWidget(m_centralWidget));
    PanelsWidget *runPanel(new PanelsWidget(m_centralWidget));

    foreach (IPanelFactory *panelFactory, ExtensionSystem::PluginManager::instance()->getObjects<IPanelFactory>()) {
        if (panelFactory->id() == QLatin1String(BUILDSETTINGS_PANEL_ID)) {
            IPropertiesPanel *panel = panelFactory->createPanel(target);
            buildPanel->addPropertiesPanel(panel);
            continue;
        }
        if (panelFactory->id() == QLatin1String(RUNSETTINGS_PANEL_ID)) {
            IPropertiesPanel *panel = panelFactory->createPanel(target);
            runPanel->addPropertiesPanel(panel);
            continue;
        }
    }
    m_centralWidget->addWidget(buildPanel);
    m_centralWidget->addWidget(runPanel);

    m_panelWidgets[0] = buildPanel;
    m_panelWidgets[1] = runPanel;

    m_centralWidget->setCurrentWidget(m_panelWidgets[subIndex]);
}

void TargetSettingsPanelWidget::addTarget()
{
    AddTargetDialog dialog(m_project);
    dialog.exec();
}

void TargetSettingsPanelWidget::removeTarget()
{
    int index = m_selector->currentIndex();
    Target *t = m_project->targets().at(index);
    // TODO: Ask before removal?
    m_project->removeTarget(t);
}

void TargetSettingsPanelWidget::targetAdded(ProjectExplorer::Target *target)
{
    Q_ASSERT(m_project == target->project());
    Q_ASSERT(m_selector);

    m_selector->addTarget(target->displayName());
    m_selector->setAddButtonEnabled(m_project->possibleTargetIds().count() > 0);
    m_selector->setRemoveButtonEnabled(m_project->targets().count() > 1);
}

void TargetSettingsPanelWidget::aboutToRemoveTarget(ProjectExplorer::Target *target)
{
    Q_ASSERT(m_project == target->project());
    Q_ASSERT(m_selector);

    int index(m_project->targets().indexOf(target));
    if (index < 0)
        return;
    m_selector->removeTarget(index);
    m_selector->setAddButtonEnabled(m_project->possibleTargetIds().count() > 0);
    m_selector->setRemoveButtonEnabled(m_project->targets().count() > 2); // target is not yet removed!
}

void TargetSettingsPanelWidget::activeTargetChanged(ProjectExplorer::Target *target)
{
    Q_ASSERT(m_project == target->project());
    Q_ASSERT(m_selector);

    int index(m_project->targets().indexOf(target));
    m_selector->markActive(index);
}
