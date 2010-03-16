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

#include "genericmakestep.h"
#include "genericprojectconstants.h"
#include "genericproject.h"
#include "generictarget.h"
#include "ui_genericmakestep.h"
#include "genericbuildconfiguration.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/gnumakeparser.h>
#include <coreplugin/variablemanager.h>
#include <utils/qtcassert.h>

#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

namespace {
const char * const GENERIC_MS_ID("GenericProjectManager.GenericMakeStep");
const char * const GENERIC_MS_DISPLAY_NAME(QT_TRANSLATE_NOOP("GenericProjectManager::Internal::GenericMakeStep",
                                                             "Make"));

const char * const BUILD_TARGETS_KEY("GenericProjectManager.GenericMakeStep.BuildTargets");
const char * const MAKE_ARGUMENTS_KEY("GenericProjectManager.GenericMakeStep.MakeArguments");
const char * const MAKE_COMMAND_KEY("GenericProjectManager.GenericMakeStep.MakeCommand");
}

GenericMakeStep::GenericMakeStep(ProjectExplorer::BuildConfiguration *bc) :
    AbstractProcessStep(bc, QLatin1String(GENERIC_MS_ID))
{
    ctor();
}

GenericMakeStep::GenericMakeStep(ProjectExplorer::BuildConfiguration *bc, const QString &id) :
    AbstractProcessStep(bc, id)
{
    ctor();
}

GenericMakeStep::GenericMakeStep(ProjectExplorer::BuildConfiguration *bc, GenericMakeStep *bs) :
    AbstractProcessStep(bc, bs),
    m_buildTargets(bs->m_buildTargets),
    m_makeArguments(bs->m_makeArguments),
    m_makeCommand(bs->m_makeCommand)
{
    ctor();
}

void GenericMakeStep::ctor()
{
    setDisplayName(QCoreApplication::translate("GenericProjectManager::Internal::GenericMakeStep",
                   GENERIC_MS_DISPLAY_NAME));
}

GenericMakeStep::~GenericMakeStep()
{
}

GenericBuildConfiguration *GenericMakeStep::genericBuildConfiguration() const
{
    return static_cast<GenericBuildConfiguration *>(buildConfiguration());
}

bool GenericMakeStep::init()
{
    GenericBuildConfiguration *bc = genericBuildConfiguration();

    setEnabled(true);
    Core::VariableManager *vm = Core::VariableManager::instance();
    const QString rawBuildDir = bc->buildDirectory();
    const QString buildDir = vm->resolve(rawBuildDir);
    setWorkingDirectory(buildDir);

    setCommand(makeCommand());
    setArguments(replacedArguments());

    setEnvironment(bc->environment());

    setOutputParser(new ProjectExplorer::GnuMakeParser(buildDir));
    if (bc->genericTarget()->genericProject()->toolChain())
        appendOutputParser(bc->genericTarget()->genericProject()->toolChain()->outputParser());

    return AbstractProcessStep::init();
}

QVariantMap GenericMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
    map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), m_makeArguments);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), m_makeCommand);
    return map;
}

bool GenericMakeStep::fromMap(const QVariantMap &map)
{
    m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
    m_makeArguments = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toStringList();
    m_makeCommand = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();

    return BuildStep::fromMap(map);
}

QStringList GenericMakeStep::replacedArguments() const
{
    Core::VariableManager *vm = Core::VariableManager::instance();
    const QStringList targets = m_buildTargets;
    QStringList arguments = m_makeArguments;
    QStringList replacedArguments;
    foreach (const QString &arg, arguments) {
      replacedArguments.append(vm->resolve(arg));
    }
    foreach (const QString &arg, targets) {
      replacedArguments.append(vm->resolve(arg));
    }
    return replacedArguments;
}

QString GenericMakeStep::makeCommand() const
{
    QString command = m_makeCommand;
    if (command.isEmpty()) {
        GenericProject *pro = genericBuildConfiguration()->genericTarget()->genericProject();
        if (ProjectExplorer::ToolChain *toolChain = pro->toolChain())
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

ProjectExplorer::BuildStepConfigWidget *GenericMakeStep::createConfigWidget()
{
    return new GenericMakeStepConfigWidget(this);
}

bool GenericMakeStep::immutable() const
{
    return false;
}

bool GenericMakeStep::buildsTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void GenericMakeStep::setBuildTarget(const QString &target, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(target))
         old << target;
    else if(!on && old.contains(target))
        old.removeOne(target);

    m_buildTargets = old;
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
    GenericProject *pro = m_makeStep->genericBuildConfiguration()->genericTarget()->genericProject();
    foreach (const QString &target, pro->buildTargets()) {
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
    return tr("Make", "GenericMakestep display name.");
}

// TODO: Label should update when tool chain is changed
void GenericMakeStepConfigWidget::updateMakeOverrrideLabel()
{
    m_ui->makeLabel->setText(tr("Override %1:").arg(m_makeStep->makeCommand()));
}

void GenericMakeStepConfigWidget::init()
{
    updateMakeOverrrideLabel();

    QString makeCommand = m_makeStep->m_makeCommand;
    m_ui->makeLineEdit->setText(makeCommand);

    const QStringList &makeArguments = m_makeStep->m_makeArguments;
    m_ui->makeArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(makeArguments));

    // Disconnect to make the changes to the items
    disconnect(m_ui->targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));

    int count = m_ui->targetsList->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_ui->targetsList->item(i);
        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    updateDetails();
    // and connect again
    connect(m_ui->targetsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
}

void GenericMakeStepConfigWidget::updateDetails()
{
    m_summaryText = tr("<b>Make:</b> %1 %2")
                    .arg(m_makeStep->makeCommand(),
                         ProjectExplorer::Environment::joinArgumentList(m_makeStep->replacedArguments()));
    emit updateSummary();
}

QString GenericMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void GenericMakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

void GenericMakeStepConfigWidget::makeLineEditTextEdited()
{
    m_makeStep->m_makeCommand = m_ui->makeLineEdit->text();
    updateDetails();
}

void GenericMakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    m_makeStep->m_makeArguments =
            ProjectExplorer::Environment::parseCombinedArgString(m_ui->makeArgumentsLineEdit->text());
    updateDetails();
}

//
// GenericMakeStepFactory
//

GenericMakeStepFactory::GenericMakeStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

GenericMakeStepFactory::~GenericMakeStepFactory()
{
}

bool GenericMakeStepFactory::canCreate(ProjectExplorer::BuildConfiguration *parent,
                                       ProjectExplorer::StepType type,
                                       const QString &id) const
{
    Q_UNUSED(type)
    if (!qobject_cast<GenericBuildConfiguration *>(parent))
        return false;
    return id == QLatin1String(GENERIC_MS_ID);
}

ProjectExplorer::BuildStep *GenericMakeStepFactory::create(ProjectExplorer::BuildConfiguration *parent,
                                                           ProjectExplorer::StepType type,
                                                           const QString &id)
{
    if (!canCreate(parent, type, id))
        return 0;
    return new GenericMakeStep(parent);
}

bool GenericMakeStepFactory::canClone(ProjectExplorer::BuildConfiguration *parent,
                                      ProjectExplorer::StepType type,
                                      ProjectExplorer::BuildStep *source) const
{
    const QString id(source->id());
    return canCreate(parent, type, id);
}

ProjectExplorer::BuildStep *GenericMakeStepFactory::clone(ProjectExplorer::BuildConfiguration *parent,
                                                          ProjectExplorer::StepType type,
                                                          ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, type, source))
        return 0;
    GenericMakeStep *old(qobject_cast<GenericMakeStep *>(source));
    Q_ASSERT(old);
    return new GenericMakeStep(parent, old);
}

bool GenericMakeStepFactory::canRestore(ProjectExplorer::BuildConfiguration *parent,
                                        ProjectExplorer::StepType type,
                                        const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, type, id);
}

ProjectExplorer::BuildStep *GenericMakeStepFactory::restore(ProjectExplorer::BuildConfiguration *parent,
                                                            ProjectExplorer::StepType type,
                                                            const QVariantMap &map)
{
    if (!canRestore(parent, type, map))
        return 0;
    GenericMakeStep *bs(new GenericMakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList GenericMakeStepFactory::availableCreationIds(ProjectExplorer::BuildConfiguration *parent,
                                                         ProjectExplorer::StepType type) const
{
    Q_UNUSED(type)
    if (!qobject_cast<GenericBuildConfiguration *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(GENERIC_MS_ID);
}

QString GenericMakeStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(GENERIC_MS_ID))
        return QCoreApplication::translate("GenericProjectManager::Internal::GenericMakeStep",
                                           GENERIC_MS_DISPLAY_NAME);
    return QString();
}
