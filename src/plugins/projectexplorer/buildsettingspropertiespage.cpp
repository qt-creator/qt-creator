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

#include "buildsettingspropertiespage.h"
#include "buildstep.h"
#include "buildstepspage.h"
#include "project.h"
#include "buildconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QPair>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenu>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

///
/// BuildSettingsPanelFactory
///

bool BuildSettingsPanelFactory::supports(Project *project)
{
    return project->hasBuildSettings();
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
// BuildSettingsSubWidgets
///

BuildSettingsSubWidgets::~BuildSettingsSubWidgets()
{
    clear();
}

void BuildSettingsSubWidgets::addWidget(const QString &name, QWidget *widget)
{
    QLabel *label = new QLabel(this);
    label->setText(name);
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() *1.2);
    label->setFont(f);

    layout()->addWidget(label);
    layout()->addWidget(widget);

    m_labels.append(label);
    m_widgets.append(widget);
}

void BuildSettingsSubWidgets::clear()
{
    qDeleteAll(m_widgets);
    qDeleteAll(m_labels);
    m_widgets.clear();
    m_labels.clear();
}

QList<QWidget *> BuildSettingsSubWidgets::widgets() const
{
    return m_widgets;
}

BuildSettingsSubWidgets::BuildSettingsSubWidgets(QWidget *parent)
    : QWidget(parent)
{
    new QVBoxLayout(this);
    layout()->setMargin(0);
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
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, -1, 0, -1);
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(new QLabel(tr("Edit Build Configuration:"), this));
    m_buildConfigurationComboBox = new QComboBox(this);
    m_buildConfigurationComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    hbox->addWidget(m_buildConfigurationComboBox);

    m_addButton = new QPushButton(this);
    m_addButton->setText(tr("Add"));
    m_addButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addWidget(m_addButton);

    m_removeButton = new QPushButton(this);
    m_removeButton->setText(tr("Remove"));
    m_removeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addWidget(m_removeButton);
    hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    vbox->addLayout(hbox);

    m_subWidgets = new BuildSettingsSubWidgets(this);
    vbox->addWidget(m_subWidgets);

    QMenu *addButtonMenu = new QMenu(this);
    addButtonMenu->addAction(tr("Create &New"),
                             this, SLOT(createConfiguration()));
    addButtonMenu->addAction(tr("&Clone Selected"),
                             this, SLOT(cloneConfiguration()));
    m_addButton->setMenu(addButtonMenu);

    m_buildConfiguration = m_project->activeBuildConfiguration()->name();

    connect(m_buildConfigurationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentIndexChanged(int)));

    connect(m_removeButton, SIGNAL(clicked()),
            this, SLOT(deleteConfiguration()));

    connect(m_project, SIGNAL(buildConfigurationDisplayNameChanged(const QString &)),
            this, SLOT(buildConfigurationDisplayNameChanged(const QString &)));

    updateBuildSettings();
}

void BuildSettingsWidget::buildConfigurationDisplayNameChanged(const QString &buildConfiguration)
{
    for (int i=0; i<m_buildConfigurationComboBox->count(); ++i) {
        if (m_buildConfigurationComboBox->itemData(i).toString() == buildConfiguration) {
            m_buildConfigurationComboBox->setItemText(i, m_project->buildConfiguration(buildConfiguration)->displayName());
            break;
        }
    }
}


void BuildSettingsWidget::updateBuildSettings()
{
    // TODO save position, entry from combbox

    // Delete old tree items
    m_buildConfigurationComboBox->blockSignals(true);
    m_buildConfigurationComboBox->clear();
    m_subWidgets->clear();

    // update buttons
    m_removeButton->setEnabled(m_project->buildConfigurations().size() > 1);

    // Add pages
    BuildConfigWidget *generalConfigWidget = m_project->createConfigWidget();
    m_subWidgets->addWidget(generalConfigWidget->displayName(), generalConfigWidget);

    m_subWidgets->addWidget(tr("Build Steps"), new BuildStepsPage(m_project));
    m_subWidgets->addWidget(tr("Clean Steps"), new BuildStepsPage(m_project, true));

    QList<BuildConfigWidget *> subConfigWidgets = m_project->subConfigWidgets();
    foreach (BuildConfigWidget *subConfigWidget, subConfigWidgets)
        m_subWidgets->addWidget(subConfigWidget->displayName(), subConfigWidget);

    // Add tree items
    foreach (const BuildConfiguration *bc, m_project->buildConfigurations()) {
        m_buildConfigurationComboBox->addItem(bc->displayName(), bc->name());
        if (bc->name() == m_buildConfiguration)
            m_buildConfigurationComboBox->setCurrentIndex(m_buildConfigurationComboBox->count() - 1);
    }

    m_buildConfigurationComboBox->blockSignals(false);

    // TODO Restore position, entry from combbox
    // TODO? select entry from combobox ?
    activeBuildConfigurationChanged();
}

void BuildSettingsWidget::currentIndexChanged(int index)
{
    m_buildConfiguration = m_buildConfigurationComboBox->itemData(index).toString();
    activeBuildConfigurationChanged();
}

void BuildSettingsWidget::activeBuildConfigurationChanged()
{
    for (int i = 0; i < m_buildConfigurationComboBox->count(); ++i) {
        if (m_buildConfigurationComboBox->itemData(i).toString() == m_buildConfiguration) {
            m_buildConfigurationComboBox->setCurrentIndex(i);
            break;
        }
    }
    foreach (QWidget *widget, m_subWidgets->widgets()) {
        if (BuildConfigWidget *buildStepWidget = qobject_cast<BuildConfigWidget*>(widget)) {
            buildStepWidget->init(m_buildConfiguration);
        }
    }
}

void BuildSettingsWidget::createConfiguration()
{
    // TODO!
//    bool ok;
//    QString newBuildConfiguration = QInputDialog::getText(this, tr("New configuration"), tr("New Configuration Name:"), QLineEdit::Normal, QString(), &ok);
//    if (!ok || newBuildConfiguration.isEmpty())
//        return;
//
//    QString newDisplayName = newBuildConfiguration;
//    // Check that the internal name is not taken and use a different one otherwise
//    const QStringList &buildConfigurations = m_project->buildConfigurations();
//    if (buildConfigurations.contains(newBuildConfiguration)) {
//        int i = 2;
//        while (buildConfigurations.contains(newBuildConfiguration + QString::number(i)))
//            ++i;
//        newBuildConfiguration += QString::number(i);
//    }
//
//    // Check that we don't have a configuration with the same displayName
//    QStringList displayNames;
//    foreach (const QString &bc, buildConfigurations)
//        displayNames << m_project->buildConfiguration(bc)->displayName();
//
//    if (displayNames.contains(newDisplayName)) {
//        int i = 2;
//        while (displayNames.contains(newDisplayName + QString::number(i)))
//            ++i;
//        newDisplayName += QString::number(i);
//    }
//
//    if (m_project->newBuildConfiguration(newBuildConfiguration)) {
//        m_project->addBuildConfiguration(newBuildConfiguration);
//        m_project->setDisplayNameFor(newBuildConfiguration, newDisplayName);
//        m_buildConfiguration = newBuildConfiguration;
//
//        updateBuildSettings();
//    }
}

void BuildSettingsWidget::cloneConfiguration()
{
    const QString configuration = m_buildConfigurationComboBox->itemData(m_buildConfigurationComboBox->currentIndex()).toString();
    cloneConfiguration(configuration);
}

void BuildSettingsWidget::deleteConfiguration()
{
    const QString configuration = m_buildConfigurationComboBox->itemData(m_buildConfigurationComboBox->currentIndex()).toString();
    deleteConfiguration(configuration);
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
    if (m_project->buildConfiguration(newBuildConfiguration)) {
        int i = 2;
        while (m_project->buildConfiguration(newBuildConfiguration + QString::number(i)))
            ++i;
        newBuildConfiguration += QString::number(i);
    }

    // Check that we don't have a configuration with the same displayName
    QStringList displayNames;
    foreach (const BuildConfiguration *bc, m_project->buildConfigurations())
        displayNames << bc->displayName();

    if (displayNames.contains(newDisplayName)) {
        int i = 2;
        while (displayNames.contains(newDisplayName + QString::number(i)))
            ++i;
        newDisplayName += QString::number(i);
    }

    m_project->copyBuildConfiguration(sourceConfiguration, newBuildConfiguration);
    m_project->setDisplayNameFor(newBuildConfiguration, newDisplayName);

    m_buildConfiguration = newBuildConfiguration;
    updateBuildSettings();
}

void BuildSettingsWidget::deleteConfiguration(const QString &deleteConfiguration)
{
    if (deleteConfiguration.isEmpty() || m_project->buildConfigurations().size() <= 1)
        return;

    if (m_project->activeBuildConfiguration()->name() == deleteConfiguration) {
        foreach (BuildConfiguration *bc, m_project->buildConfigurations()) {
            if (bc->name() != deleteConfiguration) {
                m_project->setActiveBuildConfiguration(bc);
                break;
            }
        }
    }

    if (m_buildConfiguration == deleteConfiguration) {
        foreach (const BuildConfiguration *bc, m_project->buildConfigurations()) {
            if (bc->name() != deleteConfiguration) {
                m_buildConfiguration = bc->name();
                break;
            }
        }
    }

    m_project->removeBuildConfiguration(m_project->buildConfiguration(deleteConfiguration));

    updateBuildSettings();
}
