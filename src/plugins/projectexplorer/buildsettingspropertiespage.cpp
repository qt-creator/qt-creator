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

#include "buildsettingspropertiespage.h"
#include "buildstep.h"
#include "buildstepspage.h"
#include "project.h"

#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QMenu>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

///
/// BuildSettingsPanelFactory
///

bool BuildSettingsPanelFactory::supports(Project * /* project */)
{
    return true;
}

PropertiesPanel *BuildSettingsPanelFactory::createPanel(Project *project)
{
    return new BuildSettingsPanel(project);
}

///
/// BuildSettingsPanel
///

BuildSettingsPanel::BuildSettingsPanel(Project *project)
        : PropertiesPanel(),
          m_widget(new BuildSettingsWidget(project))
{
}

BuildSettingsPanel::~BuildSettingsPanel()
{
    delete m_widget;
}

QString BuildSettingsPanel::name() const
{
    return tr("Build Settings");
}

QWidget *BuildSettingsPanel::widget()
{
    return m_widget;
}

///
/// BuildSettingsWidget
///

BuildSettingsWidget::~BuildSettingsWidget()
{
}

BuildSettingsWidget::BuildSettingsWidget(Project *project)
    : m_project(project)
{
    m_ui.setupUi(this);
    m_ui.splitter->setStretchFactor(1,10);
    m_ui.buildSettingsList->setContextMenuPolicy(Qt::CustomContextMenu);

    m_ui.addButton->setIcon(QIcon(Core::Constants::ICON_PLUS));
    m_ui.addButton->setText("");
    m_ui.removeButton->setIcon(QIcon(Core::Constants::ICON_MINUS));
    m_ui.removeButton->setText("");

    QMenu *addButtonMenu = new QMenu(this);
    addButtonMenu->addAction(tr("Create &New"),
                             this, SLOT(createConfiguration()));
    addButtonMenu->addAction(tr("&Clone Selected"),
                             this, SLOT(cloneConfiguration()));
    m_ui.addButton->setMenu(addButtonMenu);


    connect(m_ui.buildSettingsList, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this, SLOT(updateSettingsWidget(QTreeWidgetItem *, QTreeWidgetItem *)));
    connect(m_ui.buildSettingsList, SIGNAL(customContextMenuRequested (const QPoint &) ),
            this, SLOT(showContextMenu(const QPoint &)));
    connect(m_ui.buildSettingsList, SIGNAL(itemChanged(QTreeWidgetItem*,int) ),
            this, SLOT(itemChanged(QTreeWidgetItem*)), Qt::QueuedConnection);

    connect(m_ui.removeButton, SIGNAL(clicked()),
            this, SLOT(deleteConfiguration()));
    connect(m_project, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(updateBuildSettings()));
    connect(m_project, SIGNAL(buildConfigurationDisplayNameChanged(const QString &)),
            this, SLOT(buildConfigurationDisplayNameChanged(const QString &)));


    // remove dummy designer widget
    while (QWidget *widget = m_ui.buildSettingsWidgets->currentWidget()) {
        m_ui.buildSettingsWidgets->removeWidget(widget);
        delete widget;
    }

    updateBuildSettings();
}

void BuildSettingsWidget::buildConfigurationDisplayNameChanged(const QString &buildConfiguration)
{
    QTreeWidgetItem *rootItem = m_ui.buildSettingsList->invisibleRootItem();
    for (int i = 0; i < rootItem->childCount(); ++i) {
        QTreeWidgetItem *child = rootItem->child(i);
        if (child->data(0, Qt::UserRole).toString() == buildConfiguration) {
            child->setText(0, m_project->displayNameFor(buildConfiguration));
            if (m_ui.buildSettingsList->currentItem() == child) {
                QWidget *widget = m_itemToWidget.value(child);
                if (BuildStepConfigWidget *buildStepWidget = qobject_cast<BuildStepConfigWidget*>(widget)) {
                    QString title;
                    title = buildStepWidget->displayName();
                    m_ui.titleLabel->setText(tr("%1 - %2").arg(m_project->displayNameFor(buildConfiguration)).arg(title));
                }
            }
        }
    }
}


void BuildSettingsWidget::updateBuildSettings()
{
    QTreeWidgetItem *rootItem = m_ui.buildSettingsList->invisibleRootItem();

    // update buttons
    m_ui.removeButton->setEnabled(m_project->buildConfigurations().size() > 1);

    // Save current selection
    QString lastCurrentItem;
    if (m_ui.buildSettingsList->currentItem())
        lastCurrentItem = m_ui.buildSettingsList->currentItem()->text(0);

    m_itemToWidget.clear();

    // Delete old tree items
    while (rootItem->childCount()) {
        QTreeWidgetItem *configPageItem = rootItem->child(0);
        rootItem->removeChild(configPageItem);
        delete configPageItem; // does that delete also subitems?
    }

    // Delete old pages
    while (m_ui.buildSettingsWidgets->count()) {
        QWidget *w = m_ui.buildSettingsWidgets->widget(0);
        m_ui.buildSettingsWidgets->removeWidget(w);
        delete w;
    }

    // Add pages
    QWidget *dummyWidget = new QWidget(this);
    QWidget *buildStepsWidget = new BuildStepsPage(m_project);
    BuildStepConfigWidget *generalConfigWidget = m_project->createConfigWidget();
    QList<BuildStepConfigWidget *> subConfigWidgets = m_project->subConfigWidgets();

    m_ui.buildSettingsWidgets->addWidget(dummyWidget);
    m_ui.buildSettingsWidgets->addWidget(buildStepsWidget);
    m_ui.buildSettingsWidgets->addWidget(generalConfigWidget);
    foreach (BuildStepConfigWidget *subConfigWidget, subConfigWidgets)
        m_ui.buildSettingsWidgets->addWidget(subConfigWidget);

    // Add tree items
    QTreeWidgetItem *activeConfigurationItem = 0;
    QString activeBuildConfiguration = m_project->activeBuildConfiguration();
    foreach (const QString &buildConfiguration, m_project->buildConfigurations()) {
        QString displayName = m_project->displayNameFor(buildConfiguration);
        QTreeWidgetItem *buildConfigItem = new QTreeWidgetItem();
        m_itemToWidget.insert(buildConfigItem, generalConfigWidget);
        buildConfigItem->setText(0, displayName);
        buildConfigItem->setData(0, Qt::UserRole, buildConfiguration);
        buildConfigItem->setCheckState(0, Qt::Unchecked);
        if (activeBuildConfiguration == buildConfiguration) {
            QFont font = buildConfigItem->font(0);
            font.setBold(true);
            buildConfigItem->setFont(0, font);
            buildConfigItem->setCheckState(0, Qt::Checked);

            activeConfigurationItem = buildConfigItem;
        }
        rootItem->addChild(buildConfigItem);

        QTreeWidgetItem *generalItem = new QTreeWidgetItem();
        m_itemToWidget.insert(generalItem, generalConfigWidget);
        generalItem->setText(0, tr("General"));
        buildConfigItem->addChild(generalItem);

        foreach (BuildStepConfigWidget *subConfigWidget, subConfigWidgets) {
            QTreeWidgetItem *subConfigItem = new QTreeWidgetItem();
            m_itemToWidget.insert(subConfigItem, subConfigWidget);
            subConfigItem->setText(0, subConfigWidget->displayName());
            buildConfigItem->addChild(subConfigItem);
        }

        QTreeWidgetItem *buildStepsItem = new QTreeWidgetItem();
        m_itemToWidget.insert(buildStepsItem, buildStepsWidget);
        buildStepsItem->setText(0, tr("Build Steps"));
        buildConfigItem->addChild(buildStepsItem);
    }

    m_ui.buildSettingsList->expandAll();

    // Restore selection
    if (!lastCurrentItem.isEmpty()) {
        for (int i = rootItem->childCount() - 1; i >= 0; --i) {
            if (rootItem->child(i)->text(0) == lastCurrentItem) {
                m_ui.buildSettingsList->setCurrentItem(rootItem->child(i));
                break;
            }
        }
    }

    if (!m_ui.buildSettingsList->currentItem()) {
        if (activeConfigurationItem)
            m_ui.buildSettingsList->setCurrentItem(activeConfigurationItem);
        else
            m_ui.buildSettingsList->setCurrentItem(m_ui.buildSettingsList->invisibleRootItem()->child(0));
    }
}

/* switch from one tree item / build step to another */
void BuildSettingsWidget::updateSettingsWidget(QTreeWidgetItem *newItem, QTreeWidgetItem *oldItem)
{
    if (oldItem == newItem)
        return;

    if (!newItem) {
        QWidget *dummyWidget = m_ui.buildSettingsWidgets->widget(0);
        m_ui.buildSettingsWidgets->setCurrentWidget(dummyWidget);
        m_ui.titleLabel->clear();
        return;
    }

    if (QWidget *widget = m_itemToWidget.value(newItem)) {
        QString buildConfiguration;
        {
            QTreeWidgetItem *configurationItem = newItem;
            while (configurationItem && configurationItem->parent())
                configurationItem = configurationItem->parent();
            if (configurationItem)
                buildConfiguration = configurationItem->data(0, Qt::UserRole).toString();
        }

        QString title;
        if (BuildStepConfigWidget *buildStepWidget = qobject_cast<BuildStepConfigWidget*>(widget)) {
            title = buildStepWidget->displayName();
            buildStepWidget->init(buildConfiguration);
        }

        m_ui.titleLabel->setText(tr("%1 - %2").arg(m_project->displayNameFor(buildConfiguration)).arg(title));
        m_ui.buildSettingsWidgets->setCurrentWidget(widget);
    }
}


void BuildSettingsWidget::showContextMenu(const QPoint &point)
{
    if (QTreeWidgetItem *item = m_ui.buildSettingsList->itemAt(point)) {
        if (!item->parent()) {
            const QString buildConfiguration = item->data(0, Qt::UserRole).toString();

            QMenu menu;
            QAction *setAsActiveAction = new QAction(tr("Set as Active"), &menu);
            QAction *cloneAction = new QAction(tr("Clone"), &menu);
            QAction *deleteAction = new QAction(tr("Delete"), &menu);

            if (m_project->activeBuildConfiguration() == buildConfiguration)
                setAsActiveAction->setEnabled(false);
            if (m_project->buildConfigurations().size() < 2)
                deleteAction->setEnabled(false);

            menu.addActions(QList<QAction*>() << setAsActiveAction << cloneAction << deleteAction);
            QPoint globalPoint = m_ui.buildSettingsList->mapToGlobal(point);
            QAction *action = menu.exec(globalPoint);
            if (action == setAsActiveAction) {
                setActiveConfiguration(buildConfiguration);
            } else if (action == cloneAction) {
                cloneConfiguration(buildConfiguration);
            } else if (action == deleteAction) {
                deleteConfiguration(buildConfiguration);
            }

            updateBuildSettings();
        }
    }
}

void BuildSettingsWidget::setActiveConfiguration()
{
    const QString configuration = m_ui.buildSettingsList->currentItem()->data(0, Qt::UserRole).toString();
    setActiveConfiguration(configuration);
}

void BuildSettingsWidget::createConfiguration()
{
    bool ok;
    QString newBuildConfiguration = QInputDialog::getText(this, tr("New configuration"), tr("New Configuration Name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || newBuildConfiguration.isEmpty())
        return;

    QString newDisplayName = newBuildConfiguration;
    // Check that the internal name is not taken and use a different one otherwise
    const QStringList &buildConfigurations = m_project->buildConfigurations();
    if (buildConfigurations.contains(newBuildConfiguration)) {
        int i = 2;
        while (buildConfigurations.contains(newBuildConfiguration + QString::number(i)))
            ++i;
        newBuildConfiguration += QString::number(i);
    }

    // Check that we don't have a configuration with the same displayName
    QStringList displayNames;
    foreach (const QString &bc, buildConfigurations)
        displayNames << m_project->displayNameFor(bc);

    if (displayNames.contains(newDisplayName)) {
        int i = 2;
        while (displayNames.contains(newDisplayName + QString::number(i)))
            ++i;
        newDisplayName += QString::number(i);
    }

    m_project->addBuildConfiguration(newBuildConfiguration);
    m_project->setDisplayNameFor(newBuildConfiguration, newDisplayName);
    m_project->newBuildConfiguration(newBuildConfiguration);
    m_project->setActiveBuildConfiguration(newBuildConfiguration);

    updateBuildSettings();
}

void BuildSettingsWidget::cloneConfiguration()
{
    QTreeWidgetItem *configItem = m_ui.buildSettingsList->currentItem();
    while (configItem->parent())
        configItem = configItem->parent();
    const QString configuration = configItem->data(0, Qt::UserRole).toString();
    cloneConfiguration(configuration);
}

void BuildSettingsWidget::deleteConfiguration()
{
    QTreeWidgetItem *configItem = m_ui.buildSettingsList->currentItem();
    while (configItem->parent())
        configItem = configItem->parent();
    const QString configuration = configItem->data(0, Qt::UserRole).toString();
    deleteConfiguration(configuration);
}

void BuildSettingsWidget::itemChanged(QTreeWidgetItem *item)
{
    // do not allow unchecking
    if (item->checkState(0) == Qt::Unchecked)
        item->setCheckState(0, Qt::Checked);
    else {
        setActiveConfiguration(item->data(0, Qt::UserRole).toString());
    }
}

void BuildSettingsWidget::setActiveConfiguration(const QString &configuration)
{
    if (configuration.isEmpty())
        return;

    m_project->setActiveBuildConfiguration(configuration);
}

void BuildSettingsWidget::cloneConfiguration(const QString &sourceConfiguration)
{
    if (sourceConfiguration.isEmpty())
        return;

    QString newBuildConfiguration = QInputDialog::getText(this, tr("Clone configuration"), tr("New Configuration Name:"));
    if (newBuildConfiguration.isEmpty())
        return;

    QString newDisplayName = newBuildConfiguration;
    // Check that the internal name is not taken and use a different one otherwise
    const QStringList &buildConfigurations = m_project->buildConfigurations();
    if (buildConfigurations.contains(newBuildConfiguration)) {
        int i = 2;
        while (buildConfigurations.contains(newBuildConfiguration + QString::number(i)))
            ++i;
        newBuildConfiguration += QString::number(i);
    }

    // Check that we don't have a configuration with the same displayName
    QStringList displayNames;
    foreach (const QString &bc, buildConfigurations)
        displayNames << m_project->displayNameFor(bc);

    if (displayNames.contains(newDisplayName)) {
        int i = 2;
        while (displayNames.contains(newDisplayName + QString::number(i)))
            ++i;
        newDisplayName += QString::number(i);
    }

    m_project->copyBuildConfiguration(sourceConfiguration, newBuildConfiguration);
    m_project->setDisplayNameFor(newBuildConfiguration, newDisplayName);
    m_project->setActiveBuildConfiguration(newBuildConfiguration);

    updateBuildSettings();
}

void BuildSettingsWidget::deleteConfiguration(const QString &deleteConfiguration)
{
    if (deleteConfiguration.isEmpty() || m_project->buildConfigurations().size() <= 1)
        return;

    if (m_project->activeBuildConfiguration() == deleteConfiguration) {
        foreach (const QString &otherConfiguration, m_project->buildConfigurations()) {
            if (otherConfiguration != deleteConfiguration) {
                m_project->setActiveBuildConfiguration(otherConfiguration);
                break;
            }
        }
    }

    m_project->removeBuildConfiguration(deleteConfiguration);

    updateBuildSettings();
}
