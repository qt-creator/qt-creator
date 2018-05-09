/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "makestep.h"
#include "ui_makestep.h"

#include "buildconfiguration.h"
#include "kitinformation.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "target.h"
#include "toolchain.h"

#include <coreplugin/id.h>
#include <utils/qtcprocess.h>

using namespace Core;

const char BUILD_TARGETS_SUFFIX[] = ".BuildTargets";
const char MAKE_ARGUMENTS_SUFFIX[] = ".MakeArguments";
const char MAKE_COMMAND_SUFFIX[] = ".MakeCommand";
const char CLEAN_SUFFIX[] = ".Clean";

namespace ProjectExplorer {

MakeStep::MakeStep(BuildStepList *parent,
                   Core::Id id,
                   const QString &buildTarget,
                   const QStringList &availableTargets)
    : AbstractProcessStep(parent, id),
      m_availableTargets(availableTargets)
{
    if (!buildTarget.isEmpty())
        setBuildTarget(buildTarget, true);
}

void MakeStep::setClean(bool clean)
{
    m_clean = clean;
}

bool MakeStep::isClean() const
{
    return m_clean;
}

void MakeStep::setMakeCommand(const QString &command)
{
    m_makeCommand = command;
}

QVariantMap MakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(id().withSuffix(BUILD_TARGETS_SUFFIX).toString(), m_buildTargets);
    map.insert(id().withSuffix(MAKE_ARGUMENTS_SUFFIX).toString(), m_makeArguments);
    map.insert(id().withSuffix(MAKE_COMMAND_SUFFIX).toString(), m_makeCommand);
    map.insert(id().withSuffix(CLEAN_SUFFIX).toString(), m_clean);
    return map;
}

bool MakeStep::fromMap(const QVariantMap &map)
{
    m_buildTargets = map.value(id().withSuffix(BUILD_TARGETS_SUFFIX).toString()).toStringList();
    m_makeArguments = map.value(id().withSuffix(MAKE_ARGUMENTS_SUFFIX).toString()).toString();
    m_makeCommand = map.value(id().withSuffix(MAKE_COMMAND_SUFFIX).toString()).toString();
    m_clean = map.value(id().withSuffix(CLEAN_SUFFIX).toString()).toBool();

    return BuildStep::fromMap(map);
}

QString MakeStep::allArguments() const
{
    QString args = m_makeArguments;
    Utils::QtcProcess::addArgs(&args, m_buildTargets);
    return args;
}

QString MakeStep::userArguments() const
{
    return m_makeArguments;
}

void MakeStep::setUserArguments(const QString &args)
{
    m_makeArguments = args;
}

QString MakeStep::makeCommand() const
{
    return m_makeCommand;
}

QString MakeStep::makeCommand(const Utils::Environment &environment) const
{
    QString command = m_makeCommand;
    if (command.isEmpty()) {
        ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        if (tc)
            command = tc->makeCommand(environment);
        else
            command = "make";
    }
    return command;
}

BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeStepConfigWidget(this);
}

bool MakeStep::immutable() const
{
    return false;
}

bool MakeStep::buildsTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void MakeStep::setBuildTarget(const QString &target, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(target))
         old << target;
    else if (!on && old.contains(target))
        old.removeOne(target);

    m_buildTargets = old;
}

QStringList MakeStep::availableTargets() const
{
    return m_availableTargets;
}

//
// GenericMakeStepConfigWidget
//

MakeStepConfigWidget::MakeStepConfigWidget(MakeStep *makeStep) :
    m_makeStep(makeStep)
{
    m_ui = new Internal::Ui::MakeStep;
    m_ui->setupUi(this);

    const auto availableTargets = makeStep->availableTargets();
    for (const QString &target : availableTargets) {
        auto item = new QListWidgetItem(target, m_ui->targetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    m_ui->makeLineEdit->setText(m_makeStep->makeCommand());
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->userArguments());
    updateMakeOverrideLabel();
    updateDetails();

    connect(m_ui->targetsList, &QListWidget::itemChanged,
            this, &MakeStepConfigWidget::itemChanged);
    connect(m_ui->makeLineEdit, &QLineEdit::textEdited,
            this, &MakeStepConfigWidget::makeLineEditTextEdited);
    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &MakeStepConfigWidget::makeArgumentsLineEditTextEdited);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &MakeStepConfigWidget::updateMakeOverrideLabel);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &MakeStepConfigWidget::updateDetails);

    connect(m_makeStep->target(), &Target::kitChanged,
            this, &MakeStepConfigWidget::updateMakeOverrideLabel);

    const auto pro = m_makeStep->target()->project();
    pro->subscribeSignal(&BuildConfiguration::environmentChanged, this, [this]() {
        if (static_cast<BuildConfiguration *>(sender())->isActive()) {
            updateMakeOverrideLabel();
            updateDetails();
        }
    });
    connect(pro, &Project::activeProjectConfigurationChanged,
            this, [this](ProjectConfiguration *pc) {
        if (pc && pc->isActive()) {
            updateMakeOverrideLabel();
            updateDetails();
        }
    });
}

MakeStepConfigWidget::~MakeStepConfigWidget()
{
    delete m_ui;
}

QString MakeStepConfigWidget::displayName() const
{
    return m_makeStep->displayName();
}

void MakeStepConfigWidget::updateMakeOverrideLabel()
{
    BuildConfiguration *bc = m_makeStep->buildConfiguration();
    if (!bc)
        bc = m_makeStep->target()->activeBuildConfiguration();

    m_ui->makeLabel->setText(tr("Override %1:").arg(QDir::toNativeSeparators(m_makeStep->makeCommand(bc->environment()))));
}

void MakeStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_makeStep->buildConfiguration();
    if (!bc)
        bc = m_makeStep->target()->activeBuildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(m_makeStep->makeCommand(bc->environment()));
    param.setArguments(m_makeStep->allArguments());
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

QString MakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void MakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

void MakeStepConfigWidget::makeLineEditTextEdited()
{
    m_makeStep->setMakeCommand(m_ui->makeLineEdit->text());
    updateDetails();
}

void MakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    m_makeStep->setUserArguments(m_ui->makeArgumentsLineEdit->text());
    updateDetails();
}

} // namespace GenericProjectManager
