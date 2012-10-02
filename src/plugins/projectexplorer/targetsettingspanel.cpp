/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "targetsettingspanel.h"

#include "buildsettingspropertiespage.h"
#include "kitoptionspage.h"
#include "project.h"
#include "projectwindow.h"
#include "runsettingspropertiespage.h"
#include "target.h"
#include "targetsettingswidget.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolTip>
#include <QVBoxLayout>
#include <QToolTip>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;


///
// TargetSettingsWidget
///

TargetSettingsPanelWidget::TargetSettingsPanelWidget(Project *project) :
    m_currentTarget(0),
    m_project(project),
    m_selector(0),
    m_centralWidget(0),
    m_lastAction(0)
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

    connect(KitManager::instance(), SIGNAL(kitsChanged()),
            this, SLOT(updateTargetAddAndRemoveButtons()));
}

TargetSettingsPanelWidget::~TargetSettingsPanelWidget()
{
}

bool TargetSettingsPanelWidget::event(QEvent *event)
{
    if (event->type() == QEvent::StatusTip) {
        QStatusTipEvent *ev = static_cast<QStatusTipEvent *>(event);
        ev->accept();

        QAction *act = m_addMenu->activeAction();
        if (act != m_lastAction)
            QToolTip::showText(QPoint(), QString());
        m_lastAction = act;
        if (act) {
            QRect actionRect = m_addMenu->actionGeometry(act);
            actionRect.translate(m_addMenu->pos());
            QPoint p = QCursor::pos();
            if (!actionRect.contains(p))
                p = actionRect.center();
            p.setY(actionRect.center().y());
            QToolTip::showText(p, ev->tip(), m_addMenu, m_addMenu->actionGeometry(act));
        } else {
            QToolTip::showText(QPoint(), QString());
        }

        return true;
    }
    return QWidget::event(event);
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
    label->setText(tr("No kit defined in this project."));
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
    connect(m_selector, SIGNAL(removeButtonClicked(int)),
            this, SLOT(removeTarget(int)));
    connect(m_selector, SIGNAL(manageButtonClicked()),
            this, SLOT(openTargetPreferences()));
    connect(m_selector, SIGNAL(toolTipRequested(QPoint,int)),
            this, SLOT(showTargetToolTip(QPoint,int)));

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

    if (targetIndex == -1 || subIndex == -1) { // no more kits!
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
    Kit *k = KitManager::instance()->find(action->data().value<Core::Id>());
    QTC_ASSERT(!m_project->target(k), return);

    Target *target = m_project->createTarget(k);
    if (!target)
        return;
    m_project->addTarget(target);
}

void TargetSettingsPanelWidget::removeTarget(int targetIndex)
{
    Target *t = m_targets.at(targetIndex);

    ProjectExplorer::BuildManager *bm = ProjectExplorerPlugin::instance()->buildManager();
    if (bm->isBuilding(t)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(tr("Cancel Build && Remove Kit"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(tr("Do Not Remove"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(tr("Remove Kit %1?").arg(t->displayName()));
        box.setText(tr("The kit <b>%1</b> is currently being built.").arg(t->displayName()));
        box.setInformativeText(tr("Do you want to cancel the build process and remove the Kit anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        bm->cancel();
    } else {
        // We don't show the generic message box on removing the target, if we showed the still building one
        int ret = QMessageBox::warning(this, tr("Qt Creator"),
                                       tr("Do you really want to remove the\n"
                                          "\"%1\" kit?").arg(t->displayName()),
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return;
    }

    m_project->removeTarget(t);

}

void TargetSettingsPanelWidget::showTargetToolTip(const QPoint &globalPos, int targetIndex)
{
    QTC_ASSERT(targetIndex >= 0 && targetIndex < m_targets.count(), return);
    Target *target = m_targets.at(targetIndex);
    QToolTip::showText(globalPos, target->kit()->toHtml());
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

    connect(target, SIGNAL(displayNameChanged()), this, SLOT(renameTarget()));
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
    Q_ASSERT(m_selector);

    int index = m_targets.indexOf(target);
    m_selector->setCurrentIndex(index);
}

void TargetSettingsPanelWidget::updateTargetAddAndRemoveButtons()
{
    if (!m_selector)
        return;

    m_addMenu->clear();

    foreach (Kit *k, KitManager::instance()->kits()) {
        if (m_project->target(k))
            continue;

        QAction *action = new QAction(k->displayName(), m_addMenu);
        action->setData(QVariant::fromValue(k->id()));
        QString errorMessage;
        if (!m_project->supportsKit(k, &errorMessage)) {
            action->setEnabled(false);
            action->setStatusTip(errorMessage);
        }

        bool inserted = false;
        foreach (QAction *existing, m_addMenu->actions()) {
            if (existing->text() > action->text()) {
                m_addMenu->insertAction(existing, action);
                inserted = true;
                break;
            }
        }
        if (!inserted)
            m_addMenu->addAction(action);
    }

    m_selector->setAddButtonEnabled(!m_addMenu->actions().isEmpty());
}

void TargetSettingsPanelWidget::renameTarget()
{
    Target *t = qobject_cast<Target *>(sender());
    if (!t)
        return;
    const int pos = m_targets.indexOf(t);
    if (pos < 0)
        return;
    m_selector->renameTarget(pos, t->displayName());
}

void TargetSettingsPanelWidget::openTargetPreferences()
{
    int targetIndex = m_selector->currentIndex();
    if (targetIndex >= 0 && targetIndex < m_targets.size()) {
        ProjectExplorer::KitOptionsPage *page =
                ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::KitOptionsPage>();
        if (page)
            page->showKit(m_targets.at(targetIndex)->kit());
    }
    Core::ICore::showOptionsDialog(QLatin1String(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY),
                                   QLatin1String(Constants::KITS_SETTINGS_PAGE_ID));
}

int TargetSettingsPanelWidget::currentSubIndex() const
{
    return m_selector->currentSubIndex();
}

void TargetSettingsPanelWidget::setCurrentSubIndex(int subIndex)
{
    m_selector->setCurrentSubIndex(subIndex);
}
