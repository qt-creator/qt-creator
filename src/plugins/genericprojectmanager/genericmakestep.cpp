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

#include "genericmakestep.h"
#include "genericprojectconstants.h"
#include "genericproject.h"
#include "ui_genericmakestep.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorer.h>
#include <utils/qtcassert.h>
#include <coreplugin/variablemanager.h>

#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

GenericMakeStep::GenericMakeStep(GenericProject *pro)
    : AbstractMakeStep(pro), m_pro(pro)
{
}

GenericMakeStep::~GenericMakeStep()
{
}

bool GenericMakeStep::init(const QString &buildConfigurationName)
{
    ProjectExplorer::BuildConfiguration *bc = m_pro->buildConfiguration(buildConfigurationName);
    const QString buildParser = m_pro->buildParser(bc);
    setBuildParser(buildParser);
    qDebug() << "*** build parser:" << buildParser;

    setEnabled(true);
    Core::VariableManager *vm = Core::VariableManager::instance();
    const QString rawBuildDir = m_pro->buildDirectory(bc);
    const QString buildDir = vm->resolve(rawBuildDir);
    setWorkingDirectory(buildDir);

    setCommand(makeCommand(buildConfigurationName));
    setArguments(replacedArguments(buildConfigurationName));

    setEnvironment(m_pro->environment(bc));
    return AbstractMakeStep::init(buildConfigurationName);
}

void GenericMakeStep::restoreFromMap(const QString &buildConfiguration, const QMap<QString, QVariant> &map)
{
    m_values[buildConfiguration].buildTargets = map.value("buildTargets").toStringList();
    m_values[buildConfiguration].makeArguments = map.value("makeArguments").toStringList();
    m_values[buildConfiguration].makeCommand = map.value("makeCommand").toString();
    ProjectExplorer::AbstractMakeStep::restoreFromMap(buildConfiguration, map);
}

void GenericMakeStep::storeIntoMap(const QString &buildConfiguration, QMap<QString, QVariant> &map)
{
    map["buildTargets"] = m_values.value(buildConfiguration).buildTargets;
    map["makeArguments"] = m_values.value(buildConfiguration).makeArguments;
    map["makeCommand"] = m_values.value(buildConfiguration).makeCommand;
    ProjectExplorer::AbstractMakeStep::storeIntoMap(buildConfiguration, map);
}

void GenericMakeStep::addBuildConfiguration(const QString & name)
{
    m_values.insert(name, GenericMakeStepSettings());
}

void GenericMakeStep::removeBuildConfiguration(const QString & name)
{
    m_values.remove(name);
}

void GenericMakeStep::copyBuildConfiguration(const QString &source, const QString &dest)
{
    m_values.insert(dest, m_values.value(source));
}


QStringList GenericMakeStep::replacedArguments(const QString &buildConfiguration) const
{
    Core::VariableManager *vm = Core::VariableManager::instance();
    const QStringList targets = m_values.value(buildConfiguration).buildTargets;
    QStringList arguments = m_values.value(buildConfiguration).makeArguments;
    QStringList replacedArguments;
    foreach (const QString &arg, arguments) {
      replacedArguments.append(vm->resolve(arg));
    }
    foreach (const QString &arg, targets) {
      replacedArguments.append(vm->resolve(arg));
    }
    return replacedArguments;
}

QString GenericMakeStep::makeCommand(const QString &buildConfiguration) const
{
    QString command = m_values.value(buildConfiguration).makeCommand;
    if (command.isEmpty()) {
        if (ProjectExplorer::ToolChain *toolChain = m_pro->toolChain())
            command = toolChain->makeCommand();
        else
            command = QLatin1String("make");
    }
    return command;
}

void GenericMakeStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

QString GenericMakeStep::name()
{
    return Constants::MAKESTEP;
}

QString GenericMakeStep::displayName()
{
    return "Make";
}

ProjectExplorer::BuildStepConfigWidget *GenericMakeStep::createConfigWidget()
{
    return new GenericMakeStepConfigWidget(this);
}

bool GenericMakeStep::immutable() const
{
    return true;
}

GenericProject *GenericMakeStep::project() const
{
    return m_pro;
}

bool GenericMakeStep::buildsTarget(const QString &buildConfiguration, const QString &target) const
{
    return m_values.value(buildConfiguration).buildTargets.contains(target);
}

void GenericMakeStep::setBuildTarget(const QString &buildConfiguration, const QString &target, bool on)
{
    QStringList old = m_values.value(buildConfiguration).buildTargets;
    if (on && !old.contains(target))
         old << target;
    else if(!on && old.contains(target))
        old.removeOne(target);

    m_values[buildConfiguration].buildTargets = old;
}

//
// GenericMakeStepConfigWidget
//

GenericMakeStepConfigWidget::GenericMakeStepConfigWidget(GenericMakeStep *makeStep)
    : m_makeStep(makeStep)
{
    m_ui = new Ui::GenericMakeStep;
    m_ui->setupUi(this);

    // TODO update this list also on rescans of the GenericLists.txt
    GenericProject *pro = m_makeStep->project();
    foreach (const QString &target, pro->targets()) {
        QListWidgetItem *item = new QListWidgetItem(target, m_ui->targetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }

    connect(m_ui->targetsList, SIGNAL(itemChanged(QListWidgetItem*)),
            this, SLOT(itemChanged(QListWidgetItem*)));
    connect(m_ui->makeLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeLineEditTextEdited()));
    connect(m_ui->makeArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeArgumentsLineEditTextEdited()));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrrideLabel()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
}

QString GenericMakeStepConfigWidget::displayName() const
{
    return "Make";
}

// TODO: Label should update when tool chain is changed
void GenericMakeStepConfigWidget::updateMakeOverrrideLabel()
{
    m_ui->makeLabel->setText(tr("Override %1:").arg(m_makeStep->makeCommand(m_buildConfiguration)));
}

void GenericMakeStepConfigWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;

    updateMakeOverrrideLabel();

    QString makeCommand = m_makeStep->m_values.value(buildConfiguration).makeCommand;
    m_ui->makeLineEdit->setText(makeCommand);

    const QStringList &makeArguments = m_makeStep->m_values.value(buildConfiguration).makeArguments;
    m_ui->makeArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(makeArguments));

    // Disconnect to make the changes to the items
    disconnect(m_ui->targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));

    int count = m_ui->targetsList->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_ui->targetsList->item(i);
        item->setCheckState(m_makeStep->buildsTarget(buildConfiguration, item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    updateDetails();
    // and connect again
    connect(m_ui->targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
}

void GenericMakeStepConfigWidget::updateDetails()
{
    m_summaryText = tr("<b>Make:</b> %1 %2")
                    .arg(m_makeStep->makeCommand(m_buildConfiguration),
                         ProjectExplorer::Environment::joinArgumentList(m_makeStep->replacedArguments(m_buildConfiguration)));
    emit updateSummary();
}

QString GenericMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void GenericMakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    QTC_ASSERT(!m_buildConfiguration.isNull(), return);
    m_makeStep->setBuildTarget(m_buildConfiguration, item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

void GenericMakeStepConfigWidget::makeLineEditTextEdited()
{
    QTC_ASSERT(!m_buildConfiguration.isNull(), return);
    m_makeStep->m_values[m_buildConfiguration].makeCommand = m_ui->makeLineEdit->text();
    updateDetails();
}

void GenericMakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    QTC_ASSERT(!m_buildConfiguration.isNull(), return);
    m_makeStep->m_values[m_buildConfiguration].makeArguments =
                         ProjectExplorer::Environment::parseCombinedArgString(m_ui->makeArgumentsLineEdit->text());
    updateDetails();
}

//
// GenericMakeStepFactory
//

bool GenericMakeStepFactory::canCreate(const QString &name) const
{
    return (Constants::MAKESTEP == name);
}

ProjectExplorer::BuildStep *GenericMakeStepFactory::create(ProjectExplorer::Project *project, const QString &name) const
{
    Q_ASSERT(name == Constants::MAKESTEP);
    GenericProject *pro = qobject_cast<GenericProject *>(project);
    Q_ASSERT(pro);
    return new GenericMakeStep(pro);
}

QStringList GenericMakeStepFactory::canCreateForProject(ProjectExplorer::Project * /* pro */) const
{
    return QStringList();
}

QString GenericMakeStepFactory::displayNameForName(const QString & /* name */) const
{
    return "Make";
}
