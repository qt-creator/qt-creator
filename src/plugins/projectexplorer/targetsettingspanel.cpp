/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtGui/QMessageBox>
#include <QtGui/QVBoxLayout>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;


///
// TargetSettingsWidget
///

TargetSettingsPanelWidget::TargetSettingsPanelWidget(Project *project) :
    m_currentTarget(0),
    m_project(project),
    m_selector(0),
    m_centralWidget(0)
{
    Q_ASSERT(m_project);

    m_panelWidgets[0] = 0;
    m_panelWidgets[1] = 0;

    setupUi();

    connect(m_project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(targetAdded(ProjectExplorer::Target*)));
    connect(m_project, SIGNAL(aboutToRemoveTarget(ProjectExplorer::Target*)),
            this, SLOT(aboutToRemoveTarget(ProjectExplorer::Target*)));
    connect(m_project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(activeTargetChanged(ProjectExplorer::Target*)));
    connect(m_project, SIGNAL(supportedTargetIdsChanged()),
            this, SLOT(updateTargetAddAndRemoveButtons()));
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

    connect(m_selector, SIGNAL(currentChanged(int,int)),
            this, SLOT(currentTargetChanged(int,int)));

    foreach (Target *t, m_project->targets())
        targetAdded(t);

    connect(m_selector, SIGNAL(addButtonClicked()),
            this, SLOT(addTarget()));
    connect(m_selector, SIGNAL(removeButtonClicked()),
            this, SLOT(removeTarget()));

    if (m_project->activeTarget()) {
        m_selector->markActive(m_targets.indexOf(m_project->activeTarget()));
        m_selector->setCurrentIndex(m_targets.indexOf(m_project->activeTarget()));
    }

    updateTargetAddAndRemoveButtons();
}

void TargetSettingsPanelWidget::currentTargetChanged(int targetIndex, int subIndex)
{
    if (targetIndex < -1 || targetIndex >= m_targets.count())
        return;
    if (subIndex < -1 || subIndex >= 2)
        return;

    Target *target(m_targets.at(targetIndex));

    // Target was not actually changed:
    if (m_currentTarget == target) {
        if (m_panelWidgets[subIndex])
            m_centralWidget->setCurrentWidget(m_panelWidgets[subIndex]);
        else
            m_centralWidget->setCurrentWidget(m_noTargetLabel);
        return;
    }

    m_currentTarget = target;

    // Target has changed:
    if (targetIndex == -1) { // no more targets!
        m_centralWidget->setCurrentWidget(m_noTargetLabel);
        return;
    }

    PanelsWidget *buildPanel(new PanelsWidget(m_centralWidget));
    PanelsWidget *runPanel(new PanelsWidget(m_centralWidget));

    foreach (ITargetPanelFactory *panelFactory, ExtensionSystem::PluginManager::instance()->getObjects<ITargetPanelFactory>()) {
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

    m_centralWidget->setCurrentWidget(subIndex == 0 ? buildPanel : runPanel);

    delete m_panelWidgets[0];
    m_panelWidgets[0] = buildPanel;
    delete m_panelWidgets[1];
    m_panelWidgets[1] = runPanel;
}

void TargetSettingsPanelWidget::addTarget()
{
    AddTargetDialog dialog(m_project);
    dialog.exec();
}

void TargetSettingsPanelWidget::removeTarget()
{
    int index = m_selector->currentIndex();
    Target *t = m_targets.at(index);
    int ret = QMessageBox::warning(this, tr("Qt Creator"),
                                   tr("Do you really want to remove the\n"
                                      "\"%1\" target?").arg(t->displayName()),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    if (ret == QMessageBox::Yes)
        m_project->removeTarget(t);
}

void TargetSettingsPanelWidget::targetAdded(ProjectExplorer::Target *target)
{
    Q_ASSERT(m_project == target->project());
    Q_ASSERT(m_selector);

    for (int pos = 0; pos <= m_targets.count(); ++pos) {
        if (m_targets.count() == pos ||
            m_targets.at(pos)->displayName() > target->displayName()) {
            m_targets.insert(pos, target);
            m_selector->insertTarget(pos, target->displayName());
            break;
        }
    }

    updateTargetAddAndRemoveButtons();
}

void TargetSettingsPanelWidget::aboutToRemoveTarget(ProjectExplorer::Target *target)
{
    Q_ASSERT(m_project == target->project());
    Q_ASSERT(m_selector);

    int index(m_targets.indexOf(target));
    if (index < 0)
        return;
    m_targets.removeAt(index);

    m_selector->removeTarget(index);

    updateTargetAddAndRemoveButtons();
}

void TargetSettingsPanelWidget::activeTargetChanged(ProjectExplorer::Target *target)
{
    Q_ASSERT(m_project == target->project());
    Q_ASSERT(m_selector);

    int index(m_targets.indexOf(target));
    m_selector->markActive(index);
}

void TargetSettingsPanelWidget::updateTargetAddAndRemoveButtons()
{
    if (!m_selector)
        return;

    m_selector->setAddButtonEnabled(m_project->possibleTargetIds().count() > 0);
    m_selector->setRemoveButtonEnabled(m_project->targets().count() > 1);
}
