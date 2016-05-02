/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "runsettingspropertiespage.h"

#include "buildstepspage.h"
#include "deployconfiguration.h"
#include "runconfiguration.h"
#include "target.h"
#include "project.h"
#include "projectconfigurationmodel.h"
#include "session.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QVariant>
#include <QAction>
#include <QComboBox>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QWidget>

namespace ProjectExplorer {
namespace Internal {

struct FactoryAndId
{
    IRunConfigurationFactory *factory;
    Core::Id id;
};

class DeployFactoryAndId
{
public:
    DeployConfigurationFactory *factory;
    Core::Id id;
};


} // namespace Internal
} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Internal::FactoryAndId)
Q_DECLARE_METATYPE(ProjectExplorer::Internal::DeployFactoryAndId)

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using ExtensionSystem::PluginManager;

///
/// RunSettingsWidget
///

RunSettingsWidget::RunSettingsWidget(Target *target) :
    m_target(target),
    m_runConfigurationsModel(new RunConfigurationModel(target, this)),
    m_deployConfigurationModel(new DeployConfigurationModel(target, this))
{
    Q_ASSERT(m_target);

    m_deployConfigurationCombo = new QComboBox(this);
    m_addDeployToolButton = new QPushButton(tr("Add"), this);
    m_removeDeployToolButton = new QPushButton(tr("Remove"), this);
    m_renameDeployButton = new QPushButton(tr("Rename..."), this);

    auto deployWidget = new QWidget(this);

    m_runConfigurationCombo = new QComboBox(this);
    m_runConfigurationCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_runConfigurationCombo->setMinimumContentsLength(15);

    m_addRunToolButton = new QPushButton(tr("Add"), this);
    m_removeRunToolButton = new QPushButton(tr("Remove"), this);
    m_renameRunButton = new QPushButton(tr("Rename..."), this);

    auto spacer1 = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto spacer2 = new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

    auto runWidget = new QWidget(this);

    auto deployTitle = new QLabel(tr("Deployment"), this);
    auto deployLabel = new QLabel(tr("Method:"), this);
    auto runTitle = new QLabel(tr("Run"), this);
    auto runLabel = new QLabel(tr("Run configuration:"), this);

    runLabel->setBuddy(m_runConfigurationCombo);

    QFont f = runLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);

    runTitle->setFont(f);
    deployTitle->setFont(f);

    m_gridLayout = new QGridLayout(this);
    m_gridLayout->setContentsMargins(0, 20, 0, 0);
    m_gridLayout->setHorizontalSpacing(6);
    m_gridLayout->setVerticalSpacing(8);
    m_gridLayout->addWidget(deployTitle, 0, 0, 1, 6);
    m_gridLayout->addWidget(deployLabel, 1, 0, 1, 1);
    m_gridLayout->addWidget(m_deployConfigurationCombo, 1, 1, 1, 1);
    m_gridLayout->addWidget(m_addDeployToolButton, 1, 2, 1, 1);
    m_gridLayout->addWidget(m_removeDeployToolButton, 1, 3, 1, 1);
    m_gridLayout->addWidget(m_renameDeployButton, 1, 4, 1, 1);
    m_gridLayout->addWidget(deployWidget, 2, 0, 1, 6);

    m_gridLayout->addWidget(runTitle, 3, 0, 1, 6);
    m_gridLayout->addWidget(runLabel, 4, 0, 1, 1);
    m_gridLayout->addWidget(m_runConfigurationCombo, 4, 1, 1, 1);
    m_gridLayout->addWidget(m_addRunToolButton, 4, 2, 1, 1);
    m_gridLayout->addWidget(m_removeRunToolButton, 4, 3, 1, 1);
    m_gridLayout->addWidget(m_renameRunButton, 4, 4, 1, 1);
    m_gridLayout->addItem(spacer1, 4, 5, 1, 1);
    m_gridLayout->addWidget(runWidget, 5, 0, 1, 6);
    m_gridLayout->addItem(spacer2, 6, 0, 1, 1);

    // deploy part
    deployWidget->setContentsMargins(0, 10, 0, 25);
    m_deployLayout = new QVBoxLayout(deployWidget);
    m_deployLayout->setMargin(0);
    m_deployLayout->setSpacing(5);

    m_deployConfigurationCombo->setModel(m_deployConfigurationModel);

    m_addDeployMenu = new QMenu(m_addDeployToolButton);
    m_addDeployToolButton->setMenu(m_addDeployMenu);

    updateDeployConfiguration(m_target->activeDeployConfiguration());

    // Some projects may not support deployment, so we need this:
    m_addDeployToolButton->setEnabled(m_target->activeDeployConfiguration());
    m_deployConfigurationCombo->setEnabled(m_target->activeDeployConfiguration());

    m_removeDeployToolButton->setEnabled(m_target->deployConfigurations().count() > 1);
    m_renameDeployButton->setEnabled(m_target->activeDeployConfiguration());

    connect(m_addDeployMenu, &QMenu::aboutToShow,
            this, &RunSettingsWidget::aboutToShowDeployMenu);
    connect(m_deployConfigurationCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &RunSettingsWidget::currentDeployConfigurationChanged);
    connect(m_removeDeployToolButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::removeDeployConfiguration);
    connect(m_renameDeployButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::renameDeployConfiguration);

    connect(m_target, &Target::activeDeployConfigurationChanged,
            this, &RunSettingsWidget::activeDeployConfigurationChanged);

    // run part
    runWidget->setContentsMargins(0, 10, 0, 25);
    m_runLayout = new QVBoxLayout(runWidget);
    m_runLayout->setMargin(0);
    m_runLayout->setSpacing(5);

    m_disabledIcon = new QLabel;
    m_disabledIcon->setPixmap(Utils::Icons::WARNING.pixmap());
    m_disabledText = new QLabel;
    auto disabledHBox = new QHBoxLayout;
    disabledHBox->addWidget(m_disabledIcon);
    disabledHBox->addWidget(m_disabledText);
    disabledHBox->addStretch(255);

    m_runLayout->addLayout(disabledHBox);

    m_addRunMenu = new QMenu(m_addRunToolButton);
    m_addRunToolButton->setMenu(m_addRunMenu);
    RunConfiguration *rc = m_target->activeRunConfiguration();
    m_runConfigurationCombo->setModel(m_runConfigurationsModel);
    m_runConfigurationCombo->setCurrentIndex(
            m_runConfigurationsModel->indexFor(rc).row());

    m_removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
    m_renameRunButton->setEnabled(rc);

    setConfigurationWidget(rc);

    connect(m_addRunMenu, &QMenu::aboutToShow,
            this, &RunSettingsWidget::aboutToShowAddMenu);
    connect(m_runConfigurationCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &RunSettingsWidget::currentRunConfigurationChanged);
    connect(m_removeRunToolButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::removeRunConfiguration);
    connect(m_renameRunButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::renameRunConfiguration);

    connect(m_target, &Target::addedRunConfiguration,
            this, &RunSettingsWidget::updateRemoveToolButton);
    connect(m_target, &Target::removedRunConfiguration,
            this, &RunSettingsWidget::updateRemoveToolButton);

    connect(m_target, &Target::addedDeployConfiguration,
            this, &RunSettingsWidget::updateRemoveToolButton);
    connect(m_target, &Target::removedDeployConfiguration,
            this, &RunSettingsWidget::updateRemoveToolButton);

    connect(m_target, &Target::activeRunConfigurationChanged,
            this, &RunSettingsWidget::activeRunConfigurationChanged);
}

void RunSettingsWidget::aboutToShowAddMenu()
{
    m_addRunMenu->clear();
    if (m_target->activeRunConfiguration()) {
        QAction *cloneAction = m_addRunMenu->addAction(tr("&Clone Selected"));
        connect(cloneAction, &QAction::triggered,
                this, &RunSettingsWidget::cloneRunConfiguration);
    }
    QList<IRunConfigurationFactory *> factories =
        ExtensionSystem::PluginManager::getObjects<IRunConfigurationFactory>();

    QList<QAction *> menuActions;
    foreach (IRunConfigurationFactory *factory, factories) {
        QList<Core::Id> ids = factory->availableCreationIds(m_target);
        foreach (Core::Id id, ids) {
            auto action = new QAction(factory->displayNameForId(id), m_addRunMenu);
            connect(action, &QAction::triggered, [factory, id, this]() {
                RunConfiguration *newRC = factory->create(m_target, id);
                if (!newRC)
                    return;
                QTC_CHECK(newRC->id() == id);
                m_target->addRunConfiguration(newRC);
                m_target->setActiveRunConfiguration(newRC);
                m_removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
            });
            menuActions.append(action);
        }
    }

    Utils::sort(menuActions, &QAction::text);
    foreach (QAction *action, menuActions)
        m_addRunMenu->addAction(action);
}

void RunSettingsWidget::cloneRunConfiguration()
{
    RunConfiguration* activeRunConfiguration = m_target->activeRunConfiguration();
    IRunConfigurationFactory *factory = IRunConfigurationFactory::find(m_target,
                                                                       activeRunConfiguration);
    if (!factory)
        return;

    //: Title of a the cloned RunConfiguration window, text of the window
    QString name = uniqueRCName(
                        QInputDialog::getText(this,
                                              tr("Clone Configuration"),
                                              tr("New configuration name:"),
                                              QLineEdit::Normal,
                                              m_target->activeRunConfiguration()->displayName()));
    if (name.isEmpty())
        return;

    RunConfiguration *newRc = factory->clone(m_target, activeRunConfiguration);
    if (!newRc)
        return;

    newRc->setDisplayName(name);
    m_target->addRunConfiguration(newRc);
    m_target->setActiveRunConfiguration(newRc);
}

void RunSettingsWidget::removeRunConfiguration()
{
    RunConfiguration *rc = m_target->activeRunConfiguration();
    QMessageBox msgBox(QMessageBox::Question, tr("Remove Run Configuration?"),
                       tr("Do you really want to delete the run configuration <b>%1</b>?").arg(rc->displayName()),
                       QMessageBox::Yes|QMessageBox::No, this);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setEscapeButton(QMessageBox::No);
    if (msgBox.exec() == QMessageBox::No)
        return;

    m_target->removeRunConfiguration(rc);
    m_removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
    m_renameRunButton->setEnabled(m_target->activeRunConfiguration());
}

void RunSettingsWidget::activeRunConfigurationChanged()
{
    if (m_ignoreChange)
        return;
    QModelIndex actRc = m_runConfigurationsModel->indexFor(m_target->activeRunConfiguration());
    m_ignoreChange = true;
    m_runConfigurationCombo->setCurrentIndex(actRc.row());
    setConfigurationWidget(qobject_cast<RunConfiguration *>(m_runConfigurationsModel->projectConfigurationAt(actRc.row())));
    m_ignoreChange = false;
    m_renameRunButton->setEnabled(m_target->activeRunConfiguration());
}

void RunSettingsWidget::renameRunConfiguration()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Rename..."),
                                         tr("New name for run configuration <b>%1</b>:").
                                            arg(m_target->activeRunConfiguration()->displayName()),
                                         QLineEdit::Normal,
                                         m_target->activeRunConfiguration()->displayName(), &ok);
    if (!ok)
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

    RunConfiguration *selectedRunConfiguration = nullptr;
    if (index >= 0)
        selectedRunConfiguration = qobject_cast<RunConfiguration *>(m_runConfigurationsModel->projectConfigurationAt(index));

    if (selectedRunConfiguration == m_runConfiguration)
        return;

    m_ignoreChange = true;
    m_target->setActiveRunConfiguration(selectedRunConfiguration);
    m_ignoreChange = false;

    // Update the run configuration configuration widget
    setConfigurationWidget(selectedRunConfiguration);
}

void RunSettingsWidget::currentDeployConfigurationChanged(int index)
{
    if (m_ignoreChange)
        return;
    if (index == -1)
        SessionManager::setActiveDeployConfiguration(m_target, nullptr, SetActive::Cascade);
    else
        SessionManager::setActiveDeployConfiguration(m_target,
                                                     qobject_cast<DeployConfiguration *>(m_deployConfigurationModel->projectConfigurationAt(index)),
                                                     SetActive::Cascade);
}

void RunSettingsWidget::aboutToShowDeployMenu()
{
    m_addDeployMenu->clear();
    QList<DeployConfigurationFactory *> factories = DeployConfigurationFactory::find(m_target);
    if (factories.isEmpty())
        return;

    foreach (DeployConfigurationFactory *factory, factories) {
        QList<Core::Id> ids = factory->availableCreationIds(m_target);
        foreach (Core::Id id, ids) {
            QAction *action = m_addDeployMenu->addAction(factory->displayNameForId(id));
            DeployFactoryAndId data = {factory, id};
            action->setData(QVariant::fromValue(data));
            connect(action, &QAction::triggered, [factory, id, this]() {
                if (!factory->canCreate(m_target, id))
                    return;
                DeployConfiguration *newDc = factory->create(m_target, id);
                if (!newDc)
                    return;
                QTC_CHECK(!newDc || newDc->id() == id);
                m_target->addDeployConfiguration(newDc);
                SessionManager::setActiveDeployConfiguration(m_target, newDc, SetActive::Cascade);
                m_removeDeployToolButton->setEnabled(m_target->deployConfigurations().size() > 1);
            });
        }
    }
}

void RunSettingsWidget::removeDeployConfiguration()
{
    DeployConfiguration *dc = m_target->activeDeployConfiguration();
    if (BuildManager::isBuilding(dc)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(tr("Cancel Build && Remove Deploy Configuration"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(tr("Do Not Remove"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(tr("Remove Deploy Configuration %1?").arg(dc->displayName()));
        box.setText(tr("The deploy configuration <b>%1</b> is currently being built.").arg(dc->displayName()));
        box.setInformativeText(tr("Do you want to cancel the build process and remove the Deploy Configuration anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        BuildManager::cancel();
    } else {
        QMessageBox msgBox(QMessageBox::Question, tr("Remove Deploy Configuration?"),
                           tr("Do you really want to delete deploy configuration <b>%1</b>?").arg(dc->displayName()),
                           QMessageBox::Yes|QMessageBox::No, this);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;
    }

    m_target->removeDeployConfiguration(dc);

    m_removeDeployToolButton->setEnabled(m_target->deployConfigurations().size() > 1);
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
    if (!ok)
        return;

    name = uniqueDCName(name);
    if (name.isEmpty())
        return;
    m_target->activeDeployConfiguration()->setDisplayName(name);
}

void RunSettingsWidget::updateRemoveToolButton()
{
    m_removeDeployToolButton->setEnabled(m_target->deployConfigurations().count() > 1);
    m_removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
}

void RunSettingsWidget::updateDeployConfiguration(DeployConfiguration *dc)
{
    delete m_deployConfigurationWidget;
    m_deployConfigurationWidget = nullptr;
    delete m_deploySteps;
    m_deploySteps = nullptr;

    m_ignoreChange = true;
    m_deployConfigurationCombo->setCurrentIndex(-1);
    m_ignoreChange = false;

    m_renameDeployButton->setEnabled(dc);

    if (!dc)
        return;

    QModelIndex actDc = m_deployConfigurationModel->indexFor(dc);
    m_ignoreChange = true;
    m_deployConfigurationCombo->setCurrentIndex(actDc.row());
    m_ignoreChange = false;

    m_deployConfigurationWidget = dc->createConfigWidget();
    if (m_deployConfigurationWidget)
        m_deployLayout->addWidget(m_deployConfigurationWidget);

    m_deploySteps = new BuildStepListWidget;
    m_deploySteps->init(dc->stepList());
    m_deployLayout->addWidget(m_deploySteps);
}

void RunSettingsWidget::setConfigurationWidget(RunConfiguration *rc)
{
    if (rc == m_runConfiguration)
        return;

    delete m_runConfigurationWidget;
    m_runConfigurationWidget = nullptr;
    removeSubWidgets();
    if (!rc)
        return;
    m_runConfigurationWidget = rc->createConfigurationWidget();
    m_runConfiguration = rc;
    if (m_runConfigurationWidget) {
        m_runLayout->addWidget(m_runConfigurationWidget);
        updateEnabledState();
        connect(m_runConfiguration, &RunConfiguration::enabledChanged,
                m_runConfigurationWidget, [this]() { updateEnabledState(); });
    }
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
    foreach (IRunConfigurationAspect *aspect, m_runConfiguration->extraAspects()) {
        RunConfigWidget *rcw = aspect->createConfigurationWidget();
        if (rcw)
            addSubWidget(rcw);
    }
}

void RunSettingsWidget::addSubWidget(RunConfigWidget *widget)
{
    widget->setContentsMargins(0, 10, 0, 0);

    auto label = new QLabel(this);
    label->setText(widget->displayName());
    connect(widget, &RunConfigWidget::displayNameChanged,
            label, &QLabel::setText);
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);
    label->setFont(f);

    label->setContentsMargins(0, 10, 0, 0);

    QGridLayout *l = m_gridLayout;
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

void RunSettingsWidget::updateEnabledState()
{
    const bool enable = m_runConfiguration ? m_runConfiguration->isEnabled() : false;
    const QString reason = m_runConfiguration ? m_runConfiguration->disabledReason() : QString();

    m_runConfigurationWidget->setEnabled(enable);

    m_disabledIcon->setVisible(!enable && !reason.isEmpty());
    m_disabledText->setVisible(!enable && !reason.isEmpty());
    m_disabledText->setText(reason);
}
