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

#include "runsettingspropertiespage.h"
#include "runconfiguration.h"

#include "ui_runsettingspropertiespage.h"

#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QPair>

namespace ProjectExplorer {
namespace Internal {

struct FactoryAndType
{
    ProjectExplorer::IRunConfigurationFactory *factory;
    QString type;
};

} // namespace Internal
} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Internal::FactoryAndType);

namespace ProjectExplorer {
namespace Internal {

/*! A model to represent the run configurations of a project. */
class RunConfigurationsModel : public QAbstractListModel
{
public:
    RunConfigurationsModel(QObject *parent = 0)
        : QAbstractListModel(parent)
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setRunConfigurations(const QList<QSharedPointer<RunConfiguration> > &runConfigurations);
    void nameChanged(RunConfiguration *rc);

private:
    QList<QSharedPointer<RunConfiguration> > m_runConfigurations;
};

} // namespace Internal
} // namespace ProjectExplorer

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using ExtensionSystem::PluginManager;

///
/// RunSettingsPanelFactory
///

bool RunSettingsPanelFactory::supports(Project * /* project */)
{
    return true;
}

PropertiesPanel *RunSettingsPanelFactory::createPanel(Project *project)
{
    return new RunSettingsPanel(project);
}

///
/// RunSettingsPanel
///

RunSettingsPanel::RunSettingsPanel(Project *project)
   : PropertiesPanel(),
     m_widget(new RunSettingsWidget(project))
{
}

RunSettingsPanel::~RunSettingsPanel()
{
    delete m_widget;
}

QString RunSettingsPanel::name() const
{
    return tr("Run Settings");
}

QWidget *RunSettingsPanel::widget()
{
    return m_widget;
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

void RunConfigurationsModel::nameChanged(RunConfiguration *rc)
{
    for (int i = 0; i<m_runConfigurations.size(); ++i) {
        if (m_runConfigurations.at(i).data() == rc) {
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
            QSharedPointer<RunConfiguration> c = m_runConfigurations.at(row);
            return c->name();
        }
    }

    return QVariant();
}

void RunConfigurationsModel::setRunConfigurations(const QList<QSharedPointer<RunConfiguration> > &runConfigurations)
{
    m_runConfigurations = runConfigurations;
    reset();
}


///
/// RunSettingsWidget
///

RunSettingsWidget::RunSettingsWidget(Project *project)
    : m_project(project),
      m_runConfigurationsModel(new RunConfigurationsModel(this)),
      m_runConfigurationWidget(0)
{
    m_ui = new Ui::RunSettingsPropertiesPage;
    m_ui->setupUi(this);
    m_addMenu = new QMenu(m_ui->addToolButton);
    m_ui->addToolButton->setIcon(QIcon(Core::Constants::ICON_PLUS));
    m_ui->addToolButton->setMenu(m_addMenu);
    m_ui->removeToolButton->setIcon(QIcon(Core::Constants::ICON_MINUS));
    m_ui->runConfigurationCombo->setModel(m_runConfigurationsModel);

    connect(m_addMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowAddMenu()));
    connect(m_ui->runConfigurationCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(activateRunConfiguration(int)));
    connect(m_ui->removeToolButton, SIGNAL(clicked(bool)),
            this, SLOT(removeRunConfiguration()));

    initRunConfigurationComboBox();
    const QList<QSharedPointer<RunConfiguration> > runConfigurations = m_project->runConfigurations();
    for (int i=0; i<runConfigurations.size(); ++i) {
        connect(runConfigurations.at(i).data(), SIGNAL(nameChanged()),
                this, SLOT(nameChanged()));
    }

    // TODO: Add support for custom runner configuration widgets once we have some
    /*
    QList<IRunConfigurationRunner *> runners = PluginManager::instance()->getObjects<IRunConfigurationRunner>();
    foreach (IRunConfigurationRunner * runner, runners) {
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
        QStringList types = factory->canCreate(m_project);
        foreach (const QString &type, types) {
            QAction *action = m_addMenu->addAction(factory->nameForType(type));;
            FactoryAndType fat;
            fat.factory = factory;
            fat.type = type;
            QVariant v;
            v.setValue(fat);
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
    FactoryAndType fat = act->data().value<FactoryAndType>();
    QSharedPointer<RunConfiguration> newRC = fat.factory->create(m_project, fat.type);
    if (!newRC)
        return;
    m_project->addRunConfiguration(newRC);
    m_project->setActiveRunConfiguration(newRC);
    initRunConfigurationComboBox();
    connect(newRC.data(), SIGNAL(nameChanged()), this, SLOT(nameChanged()));
}

void RunSettingsWidget::removeRunConfiguration()
{
    int index = m_ui->runConfigurationCombo->currentIndex();
    QSharedPointer<RunConfiguration> rc = m_project->runConfigurations().at(index);
    disconnect(rc.data(), SIGNAL(nameChanged()), this, SLOT(nameChanged()));
    m_project->removeRunConfiguration(rc);
    initRunConfigurationComboBox();
}

void RunSettingsWidget::initRunConfigurationComboBox()
{
    const QList<QSharedPointer<RunConfiguration> > runConfigurations = m_project->runConfigurations();
    QSharedPointer<RunConfiguration> activeRunConfiguration = m_project->activeRunConfiguration();
    m_runConfigurationsModel->setRunConfigurations(runConfigurations);
    // Make sure the active run configuration is selected in the combo
    for (int i = 0; i < runConfigurations.size(); ++i) {
        if (runConfigurations.at(i) == activeRunConfiguration)
            m_ui->runConfigurationCombo->setCurrentIndex(i);
    }
    m_ui->removeToolButton->setEnabled(runConfigurations.size() > 1);
}

void RunSettingsWidget::activateRunConfiguration(int index)
{
    Q_ASSERT(m_project);
    const QList<QSharedPointer<RunConfiguration> > runConfigurations = m_project->runConfigurations();
    Q_ASSERT(index < runConfigurations.size());
    QSharedPointer<RunConfiguration> selectedRunConfiguration = runConfigurations.at(index);

    // Change the active run configuration of the project
    m_project->setActiveRunConfiguration(selectedRunConfiguration);

    // Update the run configuration configuration widget
    delete m_runConfigurationWidget;
    m_runConfigurationWidget = selectedRunConfiguration->configurationWidget();
    m_ui->groupBox->layout()->addWidget(m_runConfigurationWidget);
}

void RunSettingsWidget::nameChanged()
{
    RunConfiguration *rc = qobject_cast<RunConfiguration *>(sender());
    m_runConfigurationsModel->nameChanged(rc);
}
