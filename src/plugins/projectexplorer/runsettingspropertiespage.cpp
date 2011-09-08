/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "runsettingspropertiespage.h"

#include "buildstepspage.h"
#include "deployconfiguration.h"
#include "deployconfigurationmodel.h"
#include "runconfigurationmodel.h"
#include "runconfiguration.h"
#include "target.h"
#include "project.h"

#include "ui_runsettingspropertiespage.h"

#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QPair>
#include <QtGui/QInputDialog>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>

namespace ProjectExplorer {
namespace Internal {

struct FactoryAndId
{
    ProjectExplorer::IRunConfigurationFactory *factory;
    QString id;
};

} // namespace Internal
} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Internal::FactoryAndId)

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

bool RunSettingsPanelFactory::supports(Target *target)
{
    Q_UNUSED(target);
    return true;
}

PropertiesPanel *RunSettingsPanelFactory::createPanel(Target *target)
{
    PropertiesPanel *panel = new PropertiesPanel;
    panel->setWidget(new RunSettingsWidget(target));
    panel->setIcon(QIcon(":/projectexplorer/images/RunSettings.png"));
    panel->setDisplayName(QCoreApplication::translate("RunSettingsPanel", "Run Settings"));
    return panel;
}

///
/// RunSettingsWidget
///

RunSettingsWidget::RunSettingsWidget(Target *target)
    : m_target(target),
      m_runConfigurationsModel(new RunConfigurationModel(target, this)),
      m_deployConfigurationModel(new DeployConfigurationModel(target, this)),
      m_runConfigurationWidget(0),
      m_runLayout(0),
      m_deployConfigurationWidget(0),
      m_deployLayout(0),
      m_deploySteps(0),
      m_ignoreChange(false)
{
    Q_ASSERT(m_target);

    m_ui = new Ui::RunSettingsPropertiesPage;
    m_ui->setupUi(this);

    // deploy part
    m_ui->deployWidget->setContentsMargins(0, 10, 0, 25);
    m_deployLayout = new QVBoxLayout(m_ui->deployWidget);
    m_deployLayout->setMargin(0);
    m_deployLayout->setSpacing(5);

    m_ui->deployConfigurationCombo->setModel(m_deployConfigurationModel);
    m_addDeployMenu = new QMenu(m_ui->addDeployToolButton);
    m_ui->addDeployToolButton->setMenu(m_addDeployMenu);

    updateDeployConfiguration(m_target->activeDeployConfiguration());

    // Some projects may not support deployment, so we need this:
    m_ui->addDeployToolButton->setEnabled(m_target->activeDeployConfiguration());
    m_ui->deployConfigurationCombo->setEnabled(m_target->activeDeployConfiguration());

    m_ui->removeDeployToolButton->setEnabled(m_target->deployConfigurations().count() > 1);
    m_ui->renameDeployButton->setEnabled(m_target->activeDeployConfiguration());

    connect(m_addDeployMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowDeployMenu()));
    connect(m_ui->deployConfigurationCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentDeployConfigurationChanged(int)));
    connect(m_ui->removeDeployToolButton, SIGNAL(clicked(bool)),
            this, SLOT(removeDeployConfiguration()));
    connect(m_ui->renameDeployButton, SIGNAL(clicked()),
            this, SLOT(renameDeployConfiguration()));

    connect(m_target, SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
            this, SLOT(activeDeployConfigurationChanged()));

    // run part
    m_ui->runWidget->setContentsMargins(0, 10, 0, 25);
    m_runLayout = new QVBoxLayout(m_ui->runWidget);
    m_runLayout->setMargin(0);
    m_runLayout->setSpacing(5);

    m_addRunMenu = new QMenu(m_ui->addRunToolButton);
    m_ui->addRunToolButton->setMenu(m_addRunMenu);
    m_ui->runConfigurationCombo->setModel(m_runConfigurationsModel);
    m_ui->runConfigurationCombo->setCurrentIndex(
            m_runConfigurationsModel->indexFor(m_target->activeRunConfiguration()).row());

    m_ui->removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
    m_ui->renameRunButton->setEnabled(m_target->activeRunConfiguration());

    setConfigurationWidget(m_target->activeRunConfiguration());

    connect(m_addRunMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowAddMenu()));
    connect(m_ui->runConfigurationCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentRunConfigurationChanged(int)));
    connect(m_ui->removeRunToolButton, SIGNAL(clicked(bool)),
            this, SLOT(removeRunConfiguration()));
    connect(m_ui->renameRunButton, SIGNAL(clicked()),
            this, SLOT(renameRunConfiguration()));

    connect(m_target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(activeRunConfigurationChanged()));
}

RunSettingsWidget::~RunSettingsWidget()
{
    delete m_ui;
}

void RunSettingsWidget::aboutToShowAddMenu()
{
    m_addRunMenu->clear();
    QList<IRunConfigurationFactory *> factories =
        ExtensionSystem::PluginManager::instance()->getObjects<IRunConfigurationFactory>();
    foreach (IRunConfigurationFactory *factory, factories) {
        QStringList ids = factory->availableCreationIds(m_target);
        foreach (const QString &id, ids) {
            QAction *action = m_addRunMenu->addAction(factory->displayNameForId(id));;
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
    m_ui->removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
}

void RunSettingsWidget::removeRunConfiguration()
{
    RunConfiguration *rc = m_target->activeRunConfiguration();
    QMessageBox msgBox(QMessageBox::Question, tr("Remove Run Configuration?"),
                       tr("Do you really want to delete the run configuration <b>%1</b>?").arg(rc->displayName()),
                       QMessageBox::Yes|QMessageBox::No, this);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setEscapeButton(QMessageBox::No);
    if (!this || msgBox.exec() == QMessageBox::No)
        return;

    m_target->removeRunConfiguration(rc);
    m_ui->removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
    m_ui->renameRunButton->setEnabled(m_target->activeRunConfiguration());
}

void RunSettingsWidget::activeRunConfigurationChanged()
{
    if (m_ignoreChange)
        return;
    QModelIndex actRc = m_runConfigurationsModel->indexFor(m_target->activeRunConfiguration());
    m_ignoreChange = true;
    m_ui->runConfigurationCombo->setCurrentIndex(actRc.row());
    setConfigurationWidget(m_runConfigurationsModel->runConfigurationAt(actRc.row()));
    m_ignoreChange = false;
}

void RunSettingsWidget::renameRunConfiguration()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Rename..."),
                                         tr("New name for run configuration <b>%1</b>:").
                                            arg(m_target->activeRunConfiguration()->displayName()),
                                         QLineEdit::Normal,
                                         m_target->activeRunConfiguration()->displayName(), &ok);
    if (!ok || !this)
        return;

    name = uniqueRCName(name);
    if (name.isEmpty())
        return;

    m_target->activeRunConfiguration()->setDisplayName(name);
}

void RunSettingsWidget::currentRunConfigurationChanged(int index)
{
    if (m_ignoreChange)
        return;

    RunConfiguration *selectedRunConfiguration = 0;
    if (index >= 0)
        selectedRunConfiguration = m_runConfigurationsModel->runConfigurationAt(index);

    m_ignoreChange = true;
    m_target->setActiveRunConfiguration(selectedRunConfiguration);
    m_ignoreChange = false;

    // Update the run configuration configuration widget
    setConfigurationWidget(selectedRunConfiguration);
}

void RunSettingsWidget::currentDeployConfigurationChanged(int index)
{
    if (index == -1)
        updateDeployConfiguration(0);
    else
        m_target->setActiveDeployConfiguration(m_deployConfigurationModel->deployConfigurationAt(index));
}

void RunSettingsWidget::aboutToShowDeployMenu()
{
    m_addDeployMenu->clear();
    QStringList ids = m_target->availableDeployConfigurationIds();
    foreach (const QString &id, ids) {
        QAction *action = m_addDeployMenu->addAction(m_target->displayNameForDeployConfigurationId(id));
        action->setData(QVariant(id));
        connect(action, SIGNAL(triggered()),
                this, SLOT(addDeployConfiguration()));
    }
}

void RunSettingsWidget::addDeployConfiguration()
{
    QAction *act = qobject_cast<QAction *>(sender());
    if (!act)
        return;
    QString id = act->data().toString();
    DeployConfiguration *newDc = m_target->createDeployConfiguration(id);
    if (!newDc)
        return;
    m_target->addDeployConfiguration(newDc);
    m_target->setActiveDeployConfiguration(newDc);
    m_ui->removeDeployToolButton->setEnabled(m_target->deployConfigurations().size() > 1);
}

void RunSettingsWidget::removeDeployConfiguration()
{
    DeployConfiguration *dc = m_target->activeDeployConfiguration();
    QMessageBox msgBox(QMessageBox::Question, tr("Remove Deploy Configuration?"),
                       tr("Do you really want to delete deploy configuration <b>%1</b>?").arg(dc->displayName()),
                       QMessageBox::Yes|QMessageBox::No, this);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setEscapeButton(QMessageBox::No);
    if (!this || msgBox.exec() == QMessageBox::No)
        return;

    m_target->removeDeployConfiguration(dc);
    m_ui->removeDeployToolButton->setEnabled(m_target->deployConfigurations().size() > 1);
}

void RunSettingsWidget::activeDeployConfigurationChanged()
{
    updateDeployConfiguration(m_target->activeDeployConfiguration());
}

void RunSettingsWidget::renameDeployConfiguration()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Rename..."),
                                         tr("New name for deploy configuration <b>%1</b>:").
                                            arg(m_target->activeDeployConfiguration()->displayName()),
                                         QLineEdit::Normal,
                                         m_target->activeDeployConfiguration()->displayName(), &ok);
    if (!ok || !this)
        return;

    name = uniqueDCName(name);
    if (name.isEmpty())
        return;
    m_target->activeDeployConfiguration()->setDisplayName(name);
}

void RunSettingsWidget::updateDeployConfiguration(DeployConfiguration *dc)
{
    delete m_deployConfigurationWidget;
    m_deployConfigurationWidget = 0;
    delete m_deploySteps;
    m_deploySteps = 0;

    m_ui->deployConfigurationCombo->setCurrentIndex(-1);

    if (!dc)
        return;

    QModelIndex actDc = m_deployConfigurationModel->indexFor(dc);
    m_ui->deployConfigurationCombo->setCurrentIndex(actDc.row());

    m_deployConfigurationWidget = dc->configurationWidget();
    if (m_deployConfigurationWidget) {
        m_deployConfigurationWidget->init(dc);
        m_deployLayout->addWidget(m_deployConfigurationWidget);
    }

    m_deploySteps = new BuildStepListWidget;
    m_deploySteps->init(dc->stepList());
    m_deployLayout->addWidget(m_deploySteps);
}

void RunSettingsWidget::setConfigurationWidget(RunConfiguration *rc)
{
    delete m_runConfigurationWidget;
    m_runConfigurationWidget = 0;
    removeSubWidgets();
    if (!rc)
        return;
    m_runConfigurationWidget = rc->createConfigurationWidget();
    if (m_runConfigurationWidget)
        m_runLayout->addWidget(m_runConfigurationWidget);

    addRunControlWidgets();
}

QString RunSettingsWidget::uniqueDCName(const QString &name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList dcNames;
        foreach (DeployConfiguration *dc, m_target->deployConfigurations()) {
            if (dc == m_target->activeDeployConfiguration())
                continue;
            dcNames.append(dc->displayName());
        }
        result = Project::makeUnique(result, dcNames);
    }
    return result;
}

QString RunSettingsWidget::uniqueRCName(const QString &name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList rcNames;
        foreach (RunConfiguration *rc, m_target->runConfigurations()) {
            if (rc == m_target->activeRunConfiguration())
                continue;
            rcNames.append(rc->displayName());
        }
        result = Project::makeUnique(result, rcNames);
    }
    return result;
}

void RunSettingsWidget::addRunControlWidgets()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    foreach (IRunControlFactory *f, pm->getObjects<IRunControlFactory>()) {
        ProjectExplorer::RunConfigWidget *rcw =
            f->createConfigurationWidget(m_target->activeRunConfiguration());
        if (rcw)
            addSubWidget(rcw);
    }
}

void RunSettingsWidget::addSubWidget(RunConfigWidget *widget)
{
    widget->setContentsMargins(0, 10, 0, 0);

    QLabel *label = new QLabel(this);
    label->setText(widget->displayName());
    connect(widget, SIGNAL(displayNameChanged(QString)),
            label, SLOT(setText(QString)));
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);
    label->setFont(f);

    label->setContentsMargins(0, 10, 0, 0);

    QGridLayout *l = m_ui->gridLayout;
    l->addWidget(label, l->rowCount(), 0, 1, -1);
    l->addWidget(widget, l->rowCount(), 0, 1, -1);

    m_subWidgets.append(qMakePair(widget, label));
}

void RunSettingsWidget::removeSubWidgets()
{
    // foreach does not like commas in types, it's only a macro after all
    foreach (const RunConfigItem &item, m_subWidgets) {
        delete item.first;
        delete item.second;
    }
    m_subWidgets.clear();
}
