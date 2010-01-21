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
#include <utils/qtcassert.h>

#include <QtCore/QMargins>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
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

QString BuildSettingsPanelFactory::displayName() const
{
    return QApplication::tr("Build Settings");
}

bool BuildSettingsPanelFactory::supports(Project *project)
{
    return project->hasBuildSettings();
}

IPropertiesPanel *BuildSettingsPanelFactory::createPanel(Project *project)
{
    return new BuildSettingsPanel(project);
}

///
// BuildSettingsPanel
///

BuildSettingsPanel::BuildSettingsPanel(Project *project) :
    m_widget(new BuildSettingsWidget(project)),
    m_icon(":/projectexplorer/images/rebuild.png")
{
}

BuildSettingsPanel::~BuildSettingsPanel()
{
    delete m_widget;
}

QString BuildSettingsPanel::displayName() const
{
    return QApplication::tr("Build Settings");
}

QWidget *BuildSettingsPanel::widget() const
{
    return m_widget;
}

QIcon BuildSettingsPanel::icon() const
{
    return m_icon;
}

void BuildSettingsPanel::widgetWasAddedToLayout()
{
    m_widget->setupUi();
}

///
// BuildSettingsWidget
///

BuildSettingsWidget::~BuildSettingsWidget()
{
    clear();
}

BuildSettingsWidget::BuildSettingsWidget(Project *project) :
    m_project(project),
    m_buildConfiguration(0),
    m_leftMargin(0)
{
    // setup is done after panel has been added to layout.
    // setupUi will be called by the IPropertiesPanel implementation then.
    // this is necessary to handle the margin by hand for the up/down/delete hover
}

void BuildSettingsWidget::setupUi()
{
    // called by IPropertiesPanel implementation once the panel has been added
    QMargins margins(contentsMargins());
    m_leftMargin = margins.left();
    margins.setLeft(0);
    setContentsMargins(margins);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

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

    m_makeActiveLabel = new QLabel(this);
    m_makeActiveLabel->setContentsMargins(m_leftMargin, 4, 0, 4);
    m_makeActiveLabel->setVisible(false);
    vbox->addWidget(m_makeActiveLabel);

    m_buildConfiguration = m_project->activeBuildConfiguration();

    connect(m_makeActiveLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(makeActive()));

    connect(m_buildConfigurationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentIndexChanged(int)));

    connect(m_removeButton, SIGNAL(clicked()),
            this, SLOT(deleteConfiguration()));

    // TODO update on displayNameChange
//    connect(m_project, SIGNAL(buildConfigurationDisplayNameChanged(const QString &)),
//            this, SLOT(buildConfigurationDisplayNameChanged(const QString &)));

    connect(m_project, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(checkMakeActiveLabel()));

    connect(m_project, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    connect(m_project, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    foreach (BuildConfiguration *bc, m_project->buildConfigurations()) {
        connect(bc, SIGNAL(displayNameChanged()),
                this, SLOT(buildConfigurationDisplayNameChanged()));
    }

    if (m_project->buildConfigurationFactory())
        connect(m_project->buildConfigurationFactory(), SIGNAL(availableCreationIdsChanged()), SLOT(updateAddButtonMenu()));

    updateAddButtonMenu();
    updateBuildSettings();
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
    checkMakeActiveLabel();
}

void BuildSettingsWidget::addSubWidget(const QString &name, QWidget *widget)
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

QList<QWidget *> BuildSettingsWidget::subWidgets() const
{
    return m_subWidgets;
}

void BuildSettingsWidget::makeActive()
{
    m_project->setActiveBuildConfiguration(m_buildConfiguration);
}

void BuildSettingsWidget::updateAddButtonMenu()
{
    m_addButtonMenu->clear();
    m_addButtonMenu->addAction(tr("&Clone Selected"),
                               this, SLOT(cloneConfiguration()));
    IBuildConfigurationFactory *factory = m_project->buildConfigurationFactory();
    if (factory) {
        foreach (const QString &id, factory->availableCreationIds()) {
            QAction *action = m_addButtonMenu->addAction(factory->displayNameForId(id), this, SLOT(createConfiguration()));
            action->setData(id);
        }
    }
}

void BuildSettingsWidget::updateBuildSettings()
{
    // TODO save position, entry from combbox

    // Delete old tree items
    bool blocked = m_buildConfigurationComboBox->blockSignals(true);
    m_buildConfigurationComboBox->clear();
    clear();

    // update buttons
    m_removeButton->setEnabled(m_project->buildConfigurations().size() > 1);

    // Add pages
    BuildConfigWidget *generalConfigWidget = m_project->createConfigWidget();
    addSubWidget(generalConfigWidget->displayName(), generalConfigWidget);

    addSubWidget(tr("Build Steps"), new BuildStepsPage(m_project, false));
    addSubWidget(tr("Clean Steps"), new BuildStepsPage(m_project, true));

    QList<BuildConfigWidget *> subConfigWidgets = m_project->subConfigWidgets();
    foreach (BuildConfigWidget *subConfigWidget, subConfigWidgets)
        addSubWidget(subConfigWidget->displayName(), subConfigWidget);

    // Add tree items
    foreach (BuildConfiguration *bc, m_project->buildConfigurations()) {
        m_buildConfigurationComboBox->addItem(bc->displayName(), QVariant::fromValue<BuildConfiguration *>(bc));
        if (bc == m_buildConfiguration)
            m_buildConfigurationComboBox->setCurrentIndex(m_buildConfigurationComboBox->count() - 1);
    }

    m_buildConfigurationComboBox->blockSignals(blocked);

    activeBuildConfigurationChanged();
}

void BuildSettingsWidget::currentIndexChanged(int index)
{
    m_buildConfiguration = m_buildConfigurationComboBox->itemData(index).value<BuildConfiguration *>();
    activeBuildConfigurationChanged();
}

void BuildSettingsWidget::activeBuildConfigurationChanged()
{
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
    checkMakeActiveLabel();
}

void BuildSettingsWidget::checkMakeActiveLabel()
{
    m_makeActiveLabel->setVisible(false);
    if (!m_project->activeBuildConfiguration() || m_project->activeBuildConfiguration() != m_buildConfiguration) {
        m_makeActiveLabel->setText(tr("<a href=\"#\">Make %1 active.</a>").arg(m_buildConfiguration->displayName()));
        m_makeActiveLabel->setVisible(true);
    }
}

void BuildSettingsWidget::createConfiguration()
{
    QAction *action = qobject_cast<QAction *>(sender());
    const QString &type = action->data().toString();
    BuildConfiguration *bc = m_project->buildConfigurationFactory()->create(type);
    if (bc) {
        m_buildConfiguration = bc;
        updateBuildSettings();
    }
}

void BuildSettingsWidget::cloneConfiguration()
{
    const int index = m_buildConfigurationComboBox->currentIndex();
    BuildConfiguration *bc = m_buildConfigurationComboBox->itemData(index).value<BuildConfiguration *>();
    cloneConfiguration(bc);
}

void BuildSettingsWidget::deleteConfiguration()
{
    const int index = m_buildConfigurationComboBox->currentIndex();
    BuildConfiguration *bc = m_buildConfigurationComboBox->itemData(index).value<BuildConfiguration *>();
    deleteConfiguration(bc);
}

void BuildSettingsWidget::cloneConfiguration(BuildConfiguration *sourceConfiguration)
{
    if (!sourceConfiguration)
        return;

    QString newDisplayName(QInputDialog::getText(this, tr("Clone configuration"), tr("New Configuration Name:")));
    if (newDisplayName.isEmpty())
        return;

    QStringList buildConfigurationDisplayNames;
    foreach(BuildConfiguration *bc, m_project->buildConfigurations())
        buildConfigurationDisplayNames << bc->displayName();
    newDisplayName = Project::makeUnique(newDisplayName, buildConfigurationDisplayNames);

    m_buildConfiguration = m_project->buildConfigurationFactory()->clone(sourceConfiguration);
    m_buildConfiguration->setDisplayName(newDisplayName);
    m_project->addBuildConfiguration(m_buildConfiguration);

    updateBuildSettings();
}

void BuildSettingsWidget::deleteConfiguration(BuildConfiguration *deleteConfiguration)
{
    if (!deleteConfiguration || m_project->buildConfigurations().size() <= 1)
        return;

    if (m_project->activeBuildConfiguration() == deleteConfiguration) {
        foreach (BuildConfiguration *bc, m_project->buildConfigurations()) {
            if (bc != deleteConfiguration) {
                m_project->setActiveBuildConfiguration(bc);
                break;
            }
        }
    }

    if (m_buildConfiguration == deleteConfiguration) {
        foreach (BuildConfiguration *bc, m_project->buildConfigurations()) {
            if (bc != deleteConfiguration) {
                m_buildConfiguration = bc;
                break;
            }
        }
    }

    m_project->removeBuildConfiguration(deleteConfiguration);

    updateBuildSettings();
}
