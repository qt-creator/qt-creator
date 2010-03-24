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

#include "runsettingspropertiespage.h"
#include "runconfiguration.h"
#include "target.h"
#include "project.h"

#include "ui_runsettingspropertiespage.h"

#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QPair>
#include <QtGui/QMenu>

namespace ProjectExplorer {
namespace Internal {

struct FactoryAndId
{
    ProjectExplorer::IRunConfigurationFactory *factory;
    QString id;
};

} // namespace Internal
} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Internal::FactoryAndId);

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using ExtensionSystem::PluginManager;

///
/// RunSettingsPanelFactory
///

QString RunSettingsPanelFactory::id() const
{
    return QLatin1String(RUNSETTINGS_PANEL_ID);
}

QString RunSettingsPanelFactory::displayName() const
{
    return QCoreApplication::translate("RunSettingsPanelFactory", "Run Settings");
}

bool RunSettingsPanelFactory::supports(Project *project)
{
    return project->targets().count() == 1;
}

bool RunSettingsPanelFactory::supports(Target *target)
{
    Q_UNUSED(target);
    return true;
}

IPropertiesPanel *RunSettingsPanelFactory::createPanel(Project *project)
{
    Q_ASSERT(supports(project));
    return new RunSettingsPanel(project->activeTarget());
}

IPropertiesPanel *RunSettingsPanelFactory::createPanel(Target *target)
{
    return new RunSettingsPanel(target);
}

///
/// RunSettingsPanel
///

RunSettingsPanel::RunSettingsPanel(Target *target) :
     m_widget(new RunSettingsWidget(target)),
     m_icon(":/projectexplorer/images/RunSettings.png")
{
}

RunSettingsPanel::~RunSettingsPanel()
{
    delete m_widget;
}

QString RunSettingsPanel::displayName() const
{
    return QCoreApplication::translate("RunSettingsPanel", "Run Settings");
}

QWidget *RunSettingsPanel::widget() const
{
    return m_widget;
}

QIcon RunSettingsPanel::icon() const
{
    return m_icon;
}

///
/// RunConfigurationsModel
///

RunConfigurationsModel::RunConfigurationsModel(Target *target, QObject *parent)
    : QAbstractListModel(parent),
      m_target(target)
{
    m_runConfigurations = m_target->runConfigurations();
    connect(target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(addedRunConfiguration(ProjectExplorer::RunConfiguration*)));
    connect(target, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(removedRunConfiguration(ProjectExplorer::RunConfiguration*)));

    foreach (RunConfiguration *rc, m_runConfigurations)
        connect(rc, SIGNAL(displayNameChanged()),
                this, SLOT(displayNameChanged()));
}

int RunConfigurationsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_runConfigurations.size();
}

int RunConfigurationsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

void RunConfigurationsModel::displayNameChanged()
{
    RunConfiguration *rc = qobject_cast<RunConfiguration *>(sender());
    QTC_ASSERT(rc, return);
    for (int i = 0; i < m_runConfigurations.size(); ++i) {
        if (m_runConfigurations.at(i) == rc) {
            emit dataChanged(index(i, 0), index(i,0));
            break;
        }
    }
}

QVariant RunConfigurationsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int row = index.row();
        if (row < m_runConfigurations.size()) {
            return m_runConfigurations.at(row)->displayName();
        }
    }

    return QVariant();
}

RunConfiguration *RunConfigurationsModel::runConfigurationAt(int i)
{
    if (i > m_runConfigurations.size() || i < 0)
        return 0;
    return m_runConfigurations.at(i);
}

RunConfiguration *RunConfigurationsModel::runConfigurationFor(const QModelIndex &idx)
{
    if (idx.row() > m_runConfigurations.size() || idx.row() < 0)
        return 0;
    return m_runConfigurations.at(idx.row());
}

QModelIndex RunConfigurationsModel::indexFor(RunConfiguration *rc)
{
    int idx = m_runConfigurations.indexOf(rc);
    if (idx == -1)
        return QModelIndex();
    return index(idx, 0);
}

void RunConfigurationsModel::addedRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    int i = m_target->runConfigurations().indexOf(rc);
    QTC_ASSERT(i > 0, return);
    beginInsertRows(QModelIndex(), i, i);
    m_runConfigurations.insert(i, rc);
    endInsertRows();
    QTC_ASSERT(m_runConfigurations == m_target->runConfigurations(), return);
    connect(rc, SIGNAL(displayNameChanged()),
            this, SLOT(displayNameChanged()));
}

void RunConfigurationsModel::removedRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    int i = m_runConfigurations.indexOf(rc);
    QTC_ASSERT(i > 0, return);
    beginRemoveRows(QModelIndex(), i, i);
    m_runConfigurations.removeAt(i);
    endRemoveRows();
    QTC_ASSERT(m_runConfigurations == m_target->runConfigurations(), return);
}


///
/// RunSettingsWidget
///

RunSettingsWidget::RunSettingsWidget(Target *target)
    : m_target(target),
      m_runConfigurationsModel(new RunConfigurationsModel(target, this)),
      m_runConfigurationWidget(0),
      m_ignoreChange(false)
{
    Q_ASSERT(m_target);

    m_ui = new Ui::RunSettingsPropertiesPage;
    m_ui->setupUi(this);
    m_addMenu = new QMenu(m_ui->addToolButton);
    m_ui->addToolButton->setMenu(m_addMenu);
    m_ui->addToolButton->setText(tr("Add"));
    m_ui->removeToolButton->setText(tr("Remove"));
    m_ui->runConfigurationCombo->setModel(m_runConfigurationsModel);
    m_ui->runConfigurationCombo->setCurrentIndex(
            m_target->runConfigurations().indexOf(m_target->activeRunConfiguration()));

    m_runConfigurationWidget = m_target->activeRunConfiguration()->configurationWidget();
    layout()->addWidget(m_runConfigurationWidget);

    connect(m_addMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowAddMenu()));
    connect(m_ui->runConfigurationCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentRunConfigurationChanged(int)));
    connect(m_ui->removeToolButton, SIGNAL(clicked(bool)),
            this, SLOT(removeRunConfiguration()));

    connect(m_target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(activeRunConfigurationChanged()));

    // TODO: Add support for custom runner configuration widgets once we have some
    /*
    QList<IRunControlFactory *> runners = PluginManager::instance()->getObjects<IRunControlFactory>();
    foreach (IRunControlFactory * runner, runners) {
        if (runner->canRun(activeRunConfiguration))
            m_ui->layout->addWidget(runner->configurationWidget(activeRunConfiguration));
    }
    */
}

RunSettingsWidget::~RunSettingsWidget()
{
    delete m_ui;
}

void RunSettingsWidget::aboutToShowAddMenu()
{
    m_addMenu->clear();
    QList<IRunConfigurationFactory *> factories =
        ExtensionSystem::PluginManager::instance()->getObjects<IRunConfigurationFactory>();
    foreach (IRunConfigurationFactory *factory, factories) {
        QStringList ids = factory->availableCreationIds(m_target);
        foreach (const QString &id, ids) {
            QAction *action = m_addMenu->addAction(factory->displayNameForId(id));;
            FactoryAndId fai;
            fai.factory = factory;
            fai.id = id;
            QVariant v;
            v.setValue(fai);
            action->setData(v);
            connect(action, SIGNAL(triggered()),
                    this, SLOT(addRunConfiguration()));
        }
    }
}

void RunSettingsWidget::addRunConfiguration()
{
    QAction *act = qobject_cast<QAction *>(sender());
    if (!act)
        return;
    FactoryAndId fai = act->data().value<FactoryAndId>();
    RunConfiguration *newRC = fai.factory->create(m_target, fai.id);
    if (!newRC)
        return;
    m_target->addRunConfiguration(newRC);
    m_target->setActiveRunConfiguration(newRC);
}

void RunSettingsWidget::removeRunConfiguration()
{
    RunConfiguration *rc = m_target->activeRunConfiguration();
    m_target->removeRunConfiguration(rc);
}

void RunSettingsWidget::activeRunConfigurationChanged()
{
    if (m_ignoreChange)
        return;
    QModelIndex actRc = m_runConfigurationsModel->indexFor(m_target->activeRunConfiguration());
    m_ignoreChange = true;
    m_ui->runConfigurationCombo->setCurrentIndex(actRc.row());
    m_ignoreChange = false;

    delete m_runConfigurationWidget;
    m_runConfigurationWidget = m_target->activeRunConfiguration()->configurationWidget();
    layout()->addWidget(m_runConfigurationWidget);
}

void RunSettingsWidget::currentRunConfigurationChanged(int index)
{
    if (m_ignoreChange)
        return;
    if (index == -1) {
        delete m_runConfigurationWidget;
        m_runConfigurationWidget = 0;
        return;
    }
    RunConfiguration *selectedRunConfiguration =
            m_runConfigurationsModel->runConfigurationAt(index);

    m_ignoreChange = true;
    m_target->setActiveRunConfiguration(selectedRunConfiguration);
    m_ignoreChange = false;

    // Update the run configuration configuration widget
    delete m_runConfigurationWidget;
    m_runConfigurationWidget = selectedRunConfiguration->configurationWidget();
    layout()->addWidget(m_runConfigurationWidget);
}
