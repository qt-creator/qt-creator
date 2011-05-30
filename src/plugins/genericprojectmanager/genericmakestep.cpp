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

#include "genericmakestep.h"
#include "genericprojectconstants.h"
#include "genericproject.h"
#include "generictarget.h"
#include "ui_genericmakestep.h"
#include "genericbuildconfiguration.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/gnumakeparser.h>
#include <coreplugin/variablemanager.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

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

GenericMakeStep::GenericMakeStep(ProjectExplorer::BuildStepList *parent) :
    AbstractProcessStep(parent, QLatin1String(GENERIC_MS_ID))
{
    ctor();
}

GenericMakeStep::GenericMakeStep(ProjectExplorer::BuildStepList *parent, const QString &id) :
    AbstractProcessStep(parent, id)
{
    ctor();
}

GenericMakeStep::GenericMakeStep(ProjectExplorer::BuildStepList *parent, GenericMakeStep *bs) :
    AbstractProcessStep(parent, bs),
    m_buildTargets(bs->m_buildTargets),
    m_makeArguments(bs->m_makeArguments),
    m_makeCommand(bs->m_makeCommand)
{
    ctor();
}

void GenericMakeStep::ctor()
{
    setDefaultDisplayName(QCoreApplication::translate("GenericProjectManager::Internal::GenericMakeStep",
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
    ProjectExplorer::ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory());
    pp->setEnvironment(bc->environment());
    pp->setCommand(makeCommand());
    pp->setArguments(allArguments());

    setOutputParser(new ProjectExplorer::GnuMakeParser());
    if (bc->genericTarget()->genericProject()->toolChain())
        appendOutputParser(bc->genericTarget()->genericProject()->toolChain()->outputParser());
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

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
    m_makeArguments = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
    m_makeCommand = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();

    return BuildStep::fromMap(map);
}

QString GenericMakeStep::allArguments() const
{
    QString args = m_makeArguments;
    Utils::QtcProcess::addArgs(&args, m_buildTargets);
    return args;
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
        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    m_ui->makeLineEdit->setText(m_makeStep->m_makeCommand);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);
    updateMakeOverrrideLabel();
    updateDetails();

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

void GenericMakeStepConfigWidget::updateDetails()
{
    GenericBuildConfiguration *bc = m_makeStep->genericBuildConfiguration();

    ProjectExplorer::ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory());
    param.setEnvironment(bc->environment());
    param.setCommand(m_makeStep->makeCommand());
    param.setArguments(m_makeStep->allArguments());
    m_summaryText = param.summary(displayName());
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
    m_makeStep->m_makeArguments = m_ui->makeArgumentsLineEdit->text();
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

bool GenericMakeStepFactory::canCreate(ProjectExplorer::BuildStepList *parent,
                                       const QString &id) const
{
    if (parent->target()->project()->id() != QLatin1String(Constants::GENERICPROJECT_ID))
        return false;
    return id == QLatin1String(GENERIC_MS_ID);
}

ProjectExplorer::BuildStep *GenericMakeStepFactory::create(ProjectExplorer::BuildStepList *parent,
                                                           const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new GenericMakeStep(parent);
}

bool GenericMakeStepFactory::canClone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *source) const
{
    const QString id(source->id());
    return canCreate(parent, id);
}

ProjectExplorer::BuildStep *GenericMakeStepFactory::clone(ProjectExplorer::BuildStepList *parent,
                                                          ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    GenericMakeStep *old(qobject_cast<GenericMakeStep *>(source));
    Q_ASSERT(old);
    return new GenericMakeStep(parent, old);
}

bool GenericMakeStepFactory::canRestore(ProjectExplorer::BuildStepList *parent,
                                        const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

ProjectExplorer::BuildStep *GenericMakeStepFactory::restore(ProjectExplorer::BuildStepList *parent,
                                                            const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    GenericMakeStep *bs(new GenericMakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList GenericMakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->target()->project()->id() != QLatin1String(Constants::GENERICPROJECT_ID))
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
