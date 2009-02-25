/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "buildstepspage.h"

#include "ui_buildstepspage.h"
#include "project.h"

#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

BuildStepsPage::BuildStepsPage(Project *project) :
    BuildStepConfigWidget(),
    m_ui(new Ui::BuildStepsPage),
    m_pro(project)
{
    m_ui->setupUi(this);

    m_ui->buildStepAddButton->setMenu(new QMenu(this));
    m_ui->buildStepAddButton->setIcon(QIcon(Core::Constants::ICON_PLUS));
    m_ui->buildStepRemoveToolButton->setIcon(QIcon(Core::Constants::ICON_MINUS));
    m_ui->buildStepUpToolButton->setArrowType(Qt::UpArrow);
    m_ui->buildStepDownToolButton->setArrowType(Qt::DownArrow);

    connect(m_ui->buildSettingsList, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this, SLOT(updateBuildStepWidget(QTreeWidgetItem *, QTreeWidgetItem *)));

    connect(m_ui->buildStepAddButton->menu(), SIGNAL(aboutToShow()),
            this, SLOT(updateAddBuildStepMenu()));

    connect(m_ui->buildStepAddButton, SIGNAL(clicked()),
            this, SLOT(addBuildStep()));
    connect(m_ui->buildStepRemoveToolButton, SIGNAL(clicked()),
            this, SLOT(removeBuildStep()));
    connect(m_ui->buildStepUpToolButton, SIGNAL(clicked()),
            this, SLOT(upBuildStep()));
    connect(m_ui->buildStepDownToolButton, SIGNAL(clicked()),
            this, SLOT(downBuildStep()));

    // Remove dummy pages
    while (QWidget *widget = m_ui->buildSettingsWidget->currentWidget()) {
        m_ui->buildSettingsWidget->removeWidget(widget);
        delete widget;
    }

    // Add buildsteps
    foreach (BuildStep *bs, m_pro->buildSteps()) {

        connect(bs, SIGNAL(displayNameChanged(BuildStep *, QString)),
                this, SLOT(displayNameChanged(BuildStep *,QString)));

        QTreeWidgetItem *buildStepItem = new QTreeWidgetItem();
        buildStepItem->setText(0, bs->displayName());
        m_ui->buildSettingsWidget->addWidget(bs->createConfigWidget());
        m_ui->buildSettingsList->invisibleRootItem()->addChild(buildStepItem);
    }
}

BuildStepsPage::~BuildStepsPage()
{
    // Also deletes all added widgets
    delete m_ui;
}

void BuildStepsPage::displayNameChanged(BuildStep *bs, const QString & /* displayName */)
{
    int index = m_pro->buildSteps().indexOf(bs);
    m_ui->buildSettingsList->invisibleRootItem()->child(index)->setText(0, bs->displayName());
}

QString BuildStepsPage::displayName() const
{
    return tr("Build Steps");
}

void BuildStepsPage::init(const QString &buildConfiguration)
{
    m_configuration = buildConfiguration;

    m_ui->buildSettingsList->setCurrentItem(m_ui->buildSettingsList->invisibleRootItem()->child(0));
    // make sure widget is updated
    if (m_ui->buildSettingsWidget->currentWidget()) {
        BuildStepConfigWidget *widget = qobject_cast<BuildStepConfigWidget *>(m_ui->buildSettingsWidget->currentWidget());
        widget->init(m_configuration);
    }
}

/* switch from one tree item / build step to another */
void BuildStepsPage::updateBuildStepWidget(QTreeWidgetItem *newItem, QTreeWidgetItem *oldItem)
{
    if (oldItem == newItem)
        return;
    Q_ASSERT(m_pro);

    if (newItem) {
        int row = m_ui->buildSettingsList->indexOfTopLevelItem(newItem);
        m_ui->buildSettingsWidget->setCurrentIndex(row);
        BuildStepConfigWidget *widget = qobject_cast<BuildStepConfigWidget *>(m_ui->buildSettingsWidget->currentWidget());
        Q_ASSERT(widget);
        widget->init(m_configuration);
    }
    updateBuildStepButtonsState();
}


void BuildStepsPage::updateAddBuildStepMenu()
{
    QMap<QString, QPair<QString, IBuildStepFactory *> > map;
    //Build up a list of possible steps and save map the display names to the (internal) name and factories.
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach (IBuildStepFactory * factory, factories) {
        QStringList names = factory->canCreateForProject(m_pro);
        foreach (const QString &name, names) {
            map.insert(factory->displayNameForName(name), QPair<QString, IBuildStepFactory *>(name, factory));
        }
    }

    // Ask the user which one to add
    QMenu *menu = m_ui->buildStepAddButton->menu();
    m_addBuildStepHash.clear();
    menu->clear();
    if (!map.isEmpty()) {
        QStringList names;
        QMap<QString, QPair<QString, IBuildStepFactory *> >::const_iterator it, end;
        end = map.constEnd();
        for (it = map.constBegin(); it != end; ++it) {
            QAction *action = menu->addAction(it.key());
            connect(action, SIGNAL(triggered()),
                    this, SLOT(addBuildStep()));
            m_addBuildStepHash.insert(action, it.value());
        }
    }
}


void BuildStepsPage::addBuildStep()
{
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        QPair<QString, IBuildStepFactory *> pair = m_addBuildStepHash.value(action);
        BuildStep *newStep = pair.second->create(m_pro, pair.first);
        m_pro->insertBuildStep(0, newStep);
        QTreeWidgetItem *buildStepItem = new QTreeWidgetItem();
        buildStepItem->setText(0, newStep->displayName());
        m_ui->buildSettingsList->invisibleRootItem()->insertChild(0, buildStepItem);
        m_ui->buildSettingsWidget->insertWidget(0, newStep->createConfigWidget());
        m_ui->buildSettingsList->setCurrentItem(buildStepItem);
    }
}

void BuildStepsPage::removeBuildStep()
{
    int pos = m_ui->buildSettingsList->currentIndex().row();
    if (m_pro->buildSteps().at(pos)->immutable())
        return;
    bool blockSignals = m_ui->buildSettingsList->blockSignals(true);
    delete m_ui->buildSettingsList->invisibleRootItem()->takeChild(pos);
    m_ui->buildSettingsList->blockSignals(blockSignals);
    QWidget *widget = m_ui->buildSettingsWidget->widget(pos);
    m_ui->buildSettingsWidget->removeWidget(widget);
    delete widget;
    if (pos < m_ui->buildSettingsList->invisibleRootItem()->childCount())
        m_ui->buildSettingsList->setCurrentItem(m_ui->buildSettingsList->invisibleRootItem()->child(pos));
    else
        m_ui->buildSettingsList->setCurrentItem(m_ui->buildSettingsList->invisibleRootItem()->child(pos - 1));
    m_pro->removeBuildStep(pos);
    updateBuildStepButtonsState();
}

void BuildStepsPage::upBuildStep()
{
    int pos = m_ui->buildSettingsList->currentIndex().row();
    if (pos < 1)
        return;
    if (pos > m_ui->buildSettingsList->invisibleRootItem()->childCount()-1)
        return;
    if (m_pro->buildSteps().at(pos)->immutable() && m_pro->buildSteps().at(pos-1)->immutable())
        return;

    bool blockSignals = m_ui->buildSettingsList->blockSignals(true);
    m_pro->moveBuildStepUp(pos);
    buildStepMoveUp(pos);
    QTreeWidgetItem *item = m_ui->buildSettingsList->invisibleRootItem()->child(pos - 1);
    m_ui->buildSettingsList->blockSignals(blockSignals);
    m_ui->buildSettingsList->setCurrentItem(item);
    updateBuildStepButtonsState();
}

void BuildStepsPage::downBuildStep()
{
    int pos = m_ui->buildSettingsList->currentIndex().row() + 1;
    if (pos < 1)
        return;
    if (pos > m_ui->buildSettingsList->invisibleRootItem()->childCount() - 1)
        return;
    if (m_pro->buildSteps().at(pos)->immutable() && m_pro->buildSteps().at(pos - 1)->immutable())
        return;

    bool blockSignals = m_ui->buildSettingsList->blockSignals(true);
    m_pro->moveBuildStepUp(pos);
    buildStepMoveUp(pos);
    QTreeWidgetItem *item = m_ui->buildSettingsList->invisibleRootItem()->child(pos);
    m_ui->buildSettingsList->blockSignals(blockSignals);
    m_ui->buildSettingsList->setCurrentItem(item);
    updateBuildStepButtonsState();
}

void BuildStepsPage::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void BuildStepsPage::buildStepMoveUp(int pos)
{
    QWidget *widget = m_ui->buildSettingsWidget->widget(pos);
    m_ui->buildSettingsWidget->removeWidget(widget);
    m_ui->buildSettingsWidget->insertWidget(pos -1, widget);
    QTreeWidgetItem *item = m_ui->buildSettingsList->invisibleRootItem()->takeChild(pos);
    m_ui->buildSettingsList->invisibleRootItem()->insertChild(pos - 1, item);
}

void BuildStepsPage::updateBuildStepButtonsState()
{
    int pos = m_ui->buildSettingsList->currentIndex().row();

    m_ui->buildStepRemoveToolButton->setEnabled(!m_pro->buildSteps().at(pos)->immutable());
    bool enableUp = pos>0 && !(m_pro->buildSteps().at(pos)->immutable() && m_pro->buildSteps().at(pos-1)->immutable());
    m_ui->buildStepUpToolButton->setEnabled(enableUp);
    bool enableDown = pos < (m_ui->buildSettingsList->invisibleRootItem()->childCount() - 1) &&
                      !(m_pro->buildSteps().at(pos)->immutable() && m_pro->buildSteps().at(pos+1)->immutable());
    m_ui->buildStepDownToolButton->setEnabled(enableDown);
}
