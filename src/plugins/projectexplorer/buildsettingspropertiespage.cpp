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

#include "buildsettingspropertiespage.h"
#include "buildstep.h"
#include "buildstepspage.h"
#include "project.h"
#include "target.h"
#include "buildconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QMargins>
#include <QtCore/QTimer>
#include <QtCore/QCoreApplication>
#include <QtGui/QComboBox>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

///
// BuildSettingsPanelFactory
///

QString BuildSettingsPanelFactory::id() const
{
    return QLatin1String(BUILDSETTINGS_PANEL_ID);
}

QString BuildSettingsPanelFactory::displayName() const
{
    return QCoreApplication::translate("BuildSettingsPanelFactory", "Build Settings");
}

bool BuildSettingsPanelFactory::supports(Target *target)
{
    return target->buildConfigurationFactory();
}

IPropertiesPanel *BuildSettingsPanelFactory::createPanel(Target *target)
{
    return new BuildSettingsPanel(target);
}


///
// BuildSettingsPanel
///

BuildSettingsPanel::BuildSettingsPanel(Target *target) :
    m_widget(new BuildSettingsWidget(target)),
    m_icon(":/projectexplorer/images/BuildSettings.png")
{
}

BuildSettingsPanel::~BuildSettingsPanel()
{
    delete m_widget;
}

QString BuildSettingsPanel::displayName() const
{
    return QCoreApplication::translate("BuildSettingsPanel", "Build Settings");
}

QWidget *BuildSettingsPanel::widget() const
{
    return m_widget;
}

QIcon BuildSettingsPanel::icon() const
{
    return m_icon;
}

///
// BuildSettingsWidget
///

BuildSettingsWidget::~BuildSettingsWidget()
{
    clear();
}

BuildSettingsWidget::BuildSettingsWidget(Target *target) :
    m_target(target),
    m_buildConfiguration(0),
    m_leftMargin(0)
{
    Q_ASSERT(m_target);
    setupUi();
}

void BuildSettingsWidget::setupUi()
{
    m_leftMargin = Constants::PANEL_LEFT_MARGIN;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    if (!m_target->buildConfigurationFactory()) {
        QLabel * noSettingsLabel(new QLabel(this));
        noSettingsLabel->setText(tr("No Build Settings available"));
        {
            QFont f(noSettingsLabel->font());
            f.setPointSizeF(f.pointSizeF() * 1.2);
            noSettingsLabel->setFont(f);
        }
        vbox->addWidget(noSettingsLabel);
        return;
    }

    { // Edit Build Configuration row
        QHBoxLayout *hbox = new QHBoxLayout();
        hbox->setContentsMargins(m_leftMargin, 0, 0, 0);
        hbox->addWidget(new QLabel(tr("Edit Build Configuration:"), this));
        m_buildConfigurationComboBox = new QComboBox(this);
        m_buildConfigurationComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        hbox->addWidget(m_buildConfigurationComboBox);

        m_addButton = new QPushButton(this);
        m_addButton->setText(tr("Add"));
        m_addButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_addButton);
        m_addButtonMenu = new QMenu(this);
        m_addButton->setMenu(m_addButtonMenu);

        m_removeButton = new QPushButton(this);
        m_removeButton->setText(tr("Remove"));
        m_removeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_removeButton);

        hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        vbox->addLayout(hbox);
    }

    m_buildConfiguration = m_target->activeBuildConfiguration();

    updateAddButtonMenu();
    updateBuildSettings();

    connect(m_buildConfigurationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentIndexChanged(int)));

    connect(m_removeButton, SIGNAL(clicked()),
            this, SLOT(deleteConfiguration()));

    connect(m_target, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(updateActiveConfiguration()));

    connect(m_target, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    connect(m_target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    foreach (BuildConfiguration *bc, m_target->buildConfigurations()) {
        connect(bc, SIGNAL(displayNameChanged()),
                this, SLOT(buildConfigurationDisplayNameChanged()));
    }

    if (m_target->buildConfigurationFactory())
        connect(m_target->buildConfigurationFactory(), SIGNAL(availableCreationIdsChanged()),
                SLOT(updateAddButtonMenu()));
}

void BuildSettingsWidget::addedBuildConfiguration(BuildConfiguration *bc)
{
    connect(bc, SIGNAL(displayNameChanged()),
            this, SLOT(buildConfigurationDisplayNameChanged()));
}

void BuildSettingsWidget::removedBuildConfiguration(BuildConfiguration *bc)
{
    disconnect(bc, SIGNAL(displayNameChanged()),
               this, SLOT(buildConfigurationDisplayNameChanged()));
}

void BuildSettingsWidget::buildConfigurationDisplayNameChanged()
{
    for (int i = 0; i < m_buildConfigurationComboBox->count(); ++i) {
        BuildConfiguration *bc = m_buildConfigurationComboBox->itemData(i).value<BuildConfiguration *>();
        m_buildConfigurationComboBox->setItemText(i, bc->displayName());
    }
}

void BuildSettingsWidget::addSubWidget(const QString &name, BuildConfigWidget *widget)
{
    widget->setContentsMargins(m_leftMargin, 10, 0, 0);

    QLabel *label = new QLabel(this);
    label->setText(name);
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);
    label->setFont(f);

    label->setContentsMargins(m_leftMargin, 10, 0, 0);

    layout()->addWidget(label);
    layout()->addWidget(widget);

    m_labels.append(label);
    m_subWidgets.append(widget);
}

void BuildSettingsWidget::clear()
{
    qDeleteAll(m_subWidgets);
    m_subWidgets.clear();
    qDeleteAll(m_labels);
    m_labels.clear();
}

QList<BuildConfigWidget *> BuildSettingsWidget::subWidgets() const
{
    return m_subWidgets;
}

void BuildSettingsWidget::updateAddButtonMenu()
{
    m_addButtonMenu->clear();
    if (m_target &&
        m_target->activeBuildConfiguration()) {
        m_addButtonMenu->addAction(tr("&Clone Selected"),
                                   this, SLOT(cloneConfiguration()));
    }
    IBuildConfigurationFactory *factory = m_target->buildConfigurationFactory();
    if (factory) {
        foreach (const QString &id, factory->availableCreationIds(m_target)) {
            QAction *action = m_addButtonMenu->addAction(factory->displayNameForId(id), this, SLOT(createConfiguration()));
            action->setData(id);
        }
    }
}

void BuildSettingsWidget::updateBuildSettings()
{
    // Delete old tree items
    bool blocked = m_buildConfigurationComboBox->blockSignals(true);
    m_buildConfigurationComboBox->clear();
    clear();

    // update buttons
    m_removeButton->setEnabled(m_target->buildConfigurations().size() > 1);

    // Add pages
    BuildConfigWidget *generalConfigWidget = m_target->project()->createConfigWidget();
    addSubWidget(generalConfigWidget->displayName(), generalConfigWidget);

    addSubWidget(tr("Build Steps"), new BuildStepsPage(m_target, Build));
    addSubWidget(tr("Clean Steps"), new BuildStepsPage(m_target, Clean));

    QList<BuildConfigWidget *> subConfigWidgets = m_target->project()->subConfigWidgets();
    foreach (BuildConfigWidget *subConfigWidget, subConfigWidgets)
        addSubWidget(subConfigWidget->displayName(), subConfigWidget);

    // Add tree items
    foreach (BuildConfiguration *bc, m_target->buildConfigurations()) {
        m_buildConfigurationComboBox->addItem(bc->displayName(), QVariant::fromValue<BuildConfiguration *>(bc));
        if (bc == m_buildConfiguration)
            m_buildConfigurationComboBox->setCurrentIndex(m_buildConfigurationComboBox->count() - 1);
    }

    foreach (BuildConfigWidget *widget, subWidgets())
        widget->init(m_buildConfiguration);

    m_buildConfigurationComboBox->blockSignals(blocked);
}

void BuildSettingsWidget::currentIndexChanged(int index)
{
    BuildConfiguration *buildConfiguration = m_buildConfigurationComboBox->itemData(index).value<BuildConfiguration *>();
    m_target->setActiveBuildConfiguration(buildConfiguration);
}

void BuildSettingsWidget::updateActiveConfiguration()
{
    if (!m_buildConfiguration || m_buildConfiguration == m_target->activeBuildConfiguration())
        return;

    m_buildConfiguration = m_target->activeBuildConfiguration();

    for (int i = 0; i < m_buildConfigurationComboBox->count(); ++i) {
        if (m_buildConfigurationComboBox->itemData(i).value<BuildConfiguration *>() == m_buildConfiguration) {
            m_buildConfigurationComboBox->setCurrentIndex(i);
            break;
        }
    }

    foreach (QWidget *widget, subWidgets()) {
        if (BuildConfigWidget *buildStepWidget = qobject_cast<BuildConfigWidget*>(widget)) {
            buildStepWidget->init(m_buildConfiguration);
        }
    }
}

void BuildSettingsWidget::createConfiguration()
{
    if (!m_target->buildConfigurationFactory())
        return;

    QAction *action = qobject_cast<QAction *>(sender());
    const QString &id = action->data().toString();
    BuildConfiguration *bc = m_target->buildConfigurationFactory()->create(m_target, id);
    if (bc) {
        m_buildConfiguration = bc;
        updateBuildSettings();
    }
}

void BuildSettingsWidget::cloneConfiguration()
{
    cloneConfiguration(m_buildConfiguration);
}

void BuildSettingsWidget::deleteConfiguration()
{
    deleteConfiguration(m_buildConfiguration);
}

void BuildSettingsWidget::cloneConfiguration(BuildConfiguration *sourceConfiguration)
{
    if (!sourceConfiguration ||
        !m_target->buildConfigurationFactory())
        return;

    QString newDisplayName(QInputDialog::getText(this, tr("Clone configuration"), tr("New Configuration Name:")));
    if (newDisplayName.isEmpty())
        return;

    QStringList buildConfigurationDisplayNames;
    foreach(BuildConfiguration *bc, m_target->buildConfigurations())
        buildConfigurationDisplayNames << bc->displayName();
    newDisplayName = Project::makeUnique(newDisplayName, buildConfigurationDisplayNames);

    BuildConfiguration * bc(m_target->buildConfigurationFactory()->clone(m_target, sourceConfiguration));
    if (!bc)
        return;

    m_buildConfiguration = bc;
    m_buildConfiguration->setDisplayName(newDisplayName);
    m_target->addBuildConfiguration(m_buildConfiguration);

    updateBuildSettings();
}

void BuildSettingsWidget::deleteConfiguration(BuildConfiguration *deleteConfiguration)
{
    if (!deleteConfiguration ||
        m_target->buildConfigurations().size() <= 1)
        return;

    if (m_target->activeBuildConfiguration() == deleteConfiguration) {
        foreach (BuildConfiguration *bc, m_target->buildConfigurations()) {
            if (bc != deleteConfiguration) {
                m_target->setActiveBuildConfiguration(bc);
                break;
            }
        }
    }

    if (m_buildConfiguration == deleteConfiguration) {
        foreach (BuildConfiguration *bc, m_target->buildConfigurations()) {
            if (bc != deleteConfiguration) {
                m_buildConfiguration = bc;
                break;
            }
        }
    }

    m_target->removeBuildConfiguration(deleteConfiguration);

    updateBuildSettings();
}
