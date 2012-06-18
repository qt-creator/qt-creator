/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "targetsettingspanel.h"

#include "buildsettingspropertiespage.h"
#include "project.h"
#include "projectwindow.h"
#include "runsettingspropertiespage.h"
#include "target.h"
#include "targetsettingswidget.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>

#include <QCoreApplication>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QPushButton>

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

    m_addMenu = new QMenu(this);

    setFocusPolicy(Qt::NoFocus);

    setupUi();

    connect(m_project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(targetAdded(ProjectExplorer::Target*)));
    connect(m_project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
            this, SLOT(removedTarget(ProjectExplorer::Target*)));

    connect(m_project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(activeTargetChanged(ProjectExplorer::Target*)));

    QList<ITargetFactory *> factories =
            ExtensionSystem::PluginManager::getObjects<ITargetFactory>();

    foreach (ITargetFactory *fac, factories) {
        connect(fac, SIGNAL(canCreateTargetIdsChanged()),
                this, SLOT(updateTargetAddAndRemoveButtons()));
    }
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

    // no target label:
    m_noTargetLabel = new QWidget;
    QVBoxLayout *noTargetLayout = new QVBoxLayout(m_noTargetLabel);
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

    // Now set the correct target
    int index = m_targets.indexOf(m_project->activeTarget());
    m_selector->setCurrentIndex(index);
    m_selector->setCurrentSubIndex(0);

    currentTargetChanged(index, 0);

    connect(m_selector, SIGNAL(currentChanged(int,int)),
            this, SLOT(currentTargetChanged(int,int)));

    connect(m_selector, SIGNAL(removeButtonClicked()),
            this, SLOT(removeTarget()));

    m_selector->setAddButtonMenu(m_addMenu);
    connect(m_addMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(addTarget(QAction*)));

    updateTargetAddAndRemoveButtons();
}

void TargetSettingsPanelWidget::currentTargetChanged(int targetIndex, int subIndex)
{
    if (targetIndex < -1 || targetIndex >= m_targets.count())
        return;
    if (subIndex < -1 || subIndex >= 2)
        return;

    if (targetIndex == -1 || subIndex == -1) { // no more targets!
        delete m_panelWidgets[0];
        m_panelWidgets[0] = 0;
        delete m_panelWidgets[1];
        m_panelWidgets[1] = 0;

        m_centralWidget->setCurrentWidget(m_noTargetLabel);
        return;
    }

    Target *target = m_targets.at(targetIndex);

    // Target was not actually changed:
    if (m_currentTarget == target) {
        if (m_panelWidgets[subIndex])
            m_centralWidget->setCurrentWidget(m_panelWidgets[subIndex]);
        else
            m_centralWidget->setCurrentWidget(m_noTargetLabel);
        return;
    }

    // Target has changed:
    m_currentTarget = target;

    PanelsWidget *buildPanel = new PanelsWidget(m_centralWidget);
    PanelsWidget *runPanel = new PanelsWidget(m_centralWidget);

    foreach (ITargetPanelFactory *panelFactory, ExtensionSystem::PluginManager::getObjects<ITargetPanelFactory>()) {
        if (panelFactory->id() == QLatin1String(BUILDSETTINGS_PANEL_ID)) {
            PropertiesPanel *panel = panelFactory->createPanel(target);
            buildPanel->addPropertiesPanel(panel);
            continue;
        }
        if (panelFactory->id() == QLatin1String(RUNSETTINGS_PANEL_ID)) {
            PropertiesPanel *panel = panelFactory->createPanel(target);
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


    m_project->setActiveTarget(target);
}

void TargetSettingsPanelWidget::addTarget(QAction *action)
{
    Core::Id id = action->data().value<Core::Id>();
    Q_ASSERT(!m_project->target(id));
    QList<ITargetFactory *> factories =
            ExtensionSystem::PluginManager::getObjects<ITargetFactory>();

    Target *target = 0;
    foreach (ITargetFactory *fac, factories) {
        if (fac->canCreate(m_project, id)) {
            target = fac->create(m_project, id);
            break;
        }
    }

    if (!target)
        return;
    m_project->addTarget(target);
}

void TargetSettingsPanelWidget::removeTarget()
{
    int index = m_selector->currentIndex();
    Target *t = m_targets.at(index);

    ProjectExplorer::BuildManager *bm = ProjectExplorerPlugin::instance()->buildManager();
    if (bm->isBuilding(t)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(tr("Cancel Build && Remove Target"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(tr("Do Not Remove"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(tr("Remove Target %1?").arg(t->displayName()));
        box.setText(tr("The target <b>%1</b> is currently being built.").arg(t->displayName()));
        box.setInformativeText(tr("Do you want to cancel the build process and remove the Target anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        bm->cancel();
    } else {
        // We don't show the generic message box on removing the target, if we showed the still building one
        int ret = QMessageBox::warning(this, tr("Qt Creator"),
                                       tr("Do you really want to remove the\n"
                                          "\"%1\" target?").arg(t->displayName()),
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return;
    }

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

void TargetSettingsPanelWidget::removedTarget(ProjectExplorer::Target *target)
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

    int index = m_targets.indexOf(target);
    m_selector->setCurrentIndex(index);
}

void TargetSettingsPanelWidget::updateTargetAddAndRemoveButtons()
{
    if (!m_selector)
        return;

    m_addMenu->clear();

    QList<ITargetFactory *> factories =
            ExtensionSystem::PluginManager::getObjects<ITargetFactory>();

    foreach (ITargetFactory *fac, factories) {
        foreach (Core::Id id, fac->supportedTargetIds()) {
            if (m_project->target(id))
                continue;
            if (!fac->canCreate(m_project, id))
                continue;
            QString displayName = fac->displayNameForId(id);
            QAction *action = new QAction(displayName, m_addMenu);
            action->setData(QVariant::fromValue(id));
            bool added = false;
            foreach (QAction *existing, m_addMenu->actions()) {
                if (existing->text() > action->text()) {
                    m_addMenu->insertAction(existing, action);
                    added = true;
                }
            }

            if (!added)
                m_addMenu->addAction(action);
        }
    }

    m_selector->setAddButtonEnabled(!m_addMenu->actions().isEmpty());
    m_selector->setRemoveButtonEnabled(m_project->targets().count() > 1);
}

int TargetSettingsPanelWidget::currentSubIndex() const
{
    return m_selector->currentSubIndex();
}

void TargetSettingsPanelWidget::setCurrentSubIndex(int subIndex)
{
    m_selector->setCurrentSubIndex(subIndex);
}
