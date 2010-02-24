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

namespace ProjectExplorer {
namespace Internal {

/*! A model to represent the run configurations of a project. */
class RunConfigurationsModel : public QAbstractListModel
{
public:
    RunConfigurationsModel(QObject *parent = 0)
        : QAbstractListModel(parent),
        m_activeRunConfiguration(0)
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setRunConfigurations(const QList<RunConfiguration *> &runConfigurations);
    QList<RunConfiguration *> runConfigurations() const { return m_runConfigurations; }
    void displayNameChanged(RunConfiguration *rc);
    void activeRunConfigurationChanged(RunConfiguration *rc);

private:
    QList<RunConfiguration *> m_runConfigurations;
    RunConfiguration *m_activeRunConfiguration;
};

} // namespace Internal
} // namespace ProjectExplorer

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
    return QCoreApplication::tr("RunSettingsPanelFactory", "Run Settings");
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

int RunConfigurationsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_runConfigurations.size();
}

int RunConfigurationsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

void RunConfigurationsModel::displayNameChanged(RunConfiguration *rc)
{
    for (int i = 0; i<m_runConfigurations.size(); ++i) {
        if (m_runConfigurations.at(i) == rc) {
            emit dataChanged(index(i, 0), index(i,0));
            break;
        }
    }
}

void RunConfigurationsModel::activeRunConfigurationChanged(RunConfiguration *rc)
{
    m_activeRunConfiguration = rc;
    emit dataChanged(index(0, 0), index(m_runConfigurations.size()-1, 0));
}

QVariant RunConfigurationsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int row = index.row();
        if (row < m_runConfigurations.size()) {
            RunConfiguration *rc = m_runConfigurations.at(row);
            if (rc == m_activeRunConfiguration)
                return QCoreApplication::translate("RunConfigurationsModel", "%1 (Active)").arg(rc->displayName());
            return rc->displayName();
        }
    }

    return QVariant();
}

void RunConfigurationsModel::setRunConfigurations(const QList<RunConfiguration *> &runConfigurations)
{
    m_runConfigurations = runConfigurations;
    reset();
}


///
/// RunSettingsWidget
///

RunSettingsWidget::RunSettingsWidget(Target *target)
    : m_target(target),
      m_runConfigurationsModel(new RunConfigurationsModel(this)),
      m_runConfigurationWidget(0)
{
    Q_ASSERT(m_target);

    m_ui = new Ui::RunSettingsPropertiesPage;
    m_ui->setupUi(this);
    m_addMenu = new QMenu(m_ui->addToolButton);
    m_ui->addToolButton->setMenu(m_addMenu);
    m_ui->addToolButton->setText(tr("Add"));
    m_ui->removeToolButton->setText(tr("Remove"));
    m_ui->runConfigurationCombo->setModel(m_runConfigurationsModel);

    connect(m_addMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowAddMenu()));
    connect(m_ui->runConfigurationCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentRunConfigurationChanged(int)));
    connect(m_ui->removeToolButton, SIGNAL(clicked(bool)),
            this, SLOT(removeRunConfiguration()));
    connect(m_ui->makeActiveButton, SIGNAL(clicked()),
            this, SLOT(makeActive()));

    connect(m_target, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration *)),
            this, SLOT(initRunConfigurationComboBox()));
    connect(m_target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration *)),
            this, SLOT(initRunConfigurationComboBox()));

    connect(m_target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(activeRunConfigurationChanged()));

    initRunConfigurationComboBox();

    const QList<RunConfiguration *> runConfigurations = m_target->runConfigurations();
    for (int i=0; i<runConfigurations.size(); ++i) {
        connect(runConfigurations.at(i), SIGNAL(displayNameChanged()),
                this, SLOT(displayNameChanged()));
    }

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

RunConfiguration *RunSettingsWidget::currentRunConfiguration() const
{
    RunConfiguration *currentSelection = 0;
    const int index = m_ui->runConfigurationCombo->currentIndex();
    if (index >= 0)
        currentSelection = m_runConfigurationsModel->runConfigurations().at(index);
    return currentSelection;
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
    initRunConfigurationComboBox();
    m_ui->runConfigurationCombo->setCurrentIndex(
            m_runConfigurationsModel->runConfigurations().indexOf(newRC));
    connect(newRC, SIGNAL(displayNameChanged()), this, SLOT(displayNameChanged()));
}

void RunSettingsWidget::removeRunConfiguration()
{
    RunConfiguration *rc = currentRunConfiguration();
    disconnect(rc, SIGNAL(displayNameChanged()), this, SLOT(displayNameChanged()));
    m_target->removeRunConfiguration(rc);
    initRunConfigurationComboBox();
}

void RunSettingsWidget::makeActive()
{
    m_target->setActiveRunConfiguration(currentRunConfiguration());
}

void RunSettingsWidget::initRunConfigurationComboBox()
{
    const QList<RunConfiguration *> &runConfigurations = m_target->runConfigurations();
    RunConfiguration *activeRunConfiguration = m_target->activeRunConfiguration();
    RunConfiguration *currentSelection = currentRunConfiguration();

    m_runConfigurationsModel->setRunConfigurations(runConfigurations);
    if (runConfigurations.contains(currentSelection))
        m_ui->runConfigurationCombo->setCurrentIndex(runConfigurations.indexOf(currentSelection));
    else
        m_ui->runConfigurationCombo->setCurrentIndex(runConfigurations.indexOf(activeRunConfiguration));
    m_ui->removeToolButton->setEnabled(runConfigurations.size() > 1);
    activeRunConfigurationChanged();
}

void RunSettingsWidget::activeRunConfigurationChanged()
{
    m_runConfigurationsModel->activeRunConfigurationChanged(m_target->activeRunConfiguration());
    m_ui->makeActiveButton->setEnabled(currentRunConfiguration()
                                       && currentRunConfiguration() != m_target->activeRunConfiguration());
}

void RunSettingsWidget::currentRunConfigurationChanged(int index)
{
    m_ui->makeActiveButton->setEnabled(currentRunConfiguration()
                                       && currentRunConfiguration() != m_target->activeRunConfiguration());

    if (index == -1) {
        delete m_runConfigurationWidget;
        m_runConfigurationWidget = 0;
        return;
    }
    RunConfiguration *selectedRunConfiguration =
            m_runConfigurationsModel->runConfigurations().at(index);

    // Update the run configuration configuration widget
    delete m_runConfigurationWidget;
    m_runConfigurationWidget = selectedRunConfiguration->configurationWidget();
    layout()->addWidget(m_runConfigurationWidget);
}

void RunSettingsWidget::displayNameChanged()
{
    RunConfiguration *rc = qobject_cast<RunConfiguration *>(sender());
    m_runConfigurationsModel->displayNameChanged(rc);
}
