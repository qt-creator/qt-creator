//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
// Copyright (C) 2013 Openismus GmbH.
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the  GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#include "b2buildconfiguration.h"
#include "b2buildstep.h"
#include "b2outputparser.h"
#include "b2projectmanagerconstants.h"
#include "b2utility.h"
// Qt Creator
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
// Qt
#include <QFormLayout>
#include <QLineEdit>
#include <QScopedPointer>
#include <QString>
// std
#include <memory>

namespace BoostBuildProjectManager {
namespace Internal {

BuildStep::BuildStep(ProjectExplorer::BuildStepList* bsl)
    : ProjectExplorer::AbstractProcessStep(bsl, Core::Id(Constants::BUILDSTEP_ID))
{
}

BuildStep::BuildStep(ProjectExplorer::BuildStepList* bsl, BuildStep* bs)
    : AbstractProcessStep(bsl, bs)
    , tasks_(bs->tasks_)
    , arguments_(bs->arguments_)
{
}

BuildStep::BuildStep(ProjectExplorer::BuildStepList* bsl, Core::Id const id)
    : AbstractProcessStep(bsl, id)
{
}

bool BuildStep::init()
{
    setDefaultDisplayName(QLatin1String(Constants::BOOSTBUILD));

    tasks_.clear();
    ProjectExplorer::ToolChain* tc
        = ProjectExplorer::ToolChainKitInformation::toolChain(target()->kit());
    if (!tc) {
        BBPM_QDEBUG("Qt Creator needs compiler");
        typedef ProjectExplorer::Task Task;
        Task task(Task::Error
             , tr("Qt Creator needs a compiler set up to build. Configure a compiler in the kit options.")
             , Utils::FileName(), -1
             , ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

        tasks_.append(task);
        return !tasks_.empty(); // otherwise the tasks will not get reported
    }

    BuildConfiguration* bc = thisBuildConfiguration();
    QTC_ASSERT(bc, return false);

    setIgnoreReturnValue(Constants::ReturnValueNotIgnored);

    ProjectExplorer::ProcessParameters* pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    {
        // from GenericProjectManager code:
        // Force output to english for the parsers.
        // Do this here and not in the toolchain's addToEnvironment() to not
        // screw up the users run environment.
        Utils::Environment env = bc->environment();
        env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
        pp->setEnvironment(env);
    }
    pp->setWorkingDirectory(bc->workingDirectory().toString());
    pp->setCommand(makeCommand(bc->environment()));
    pp->setArguments(allArguments());
    pp->resolveAll();

    // Create Boost.Build parser and chain with existing parsers
    setOutputParser(new BoostBuildParser());
    if (ProjectExplorer::IOutputParser* parser = target()->kit()->createOutputParser())
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    BBPM_QDEBUG(displayName() << ", " << bc->buildDirectory().toString()
                << ", " << pp->effectiveWorkingDirectory());

    return ProjectExplorer::AbstractProcessStep::init();
}

void BuildStep::run(QFutureInterface<bool>& fi)
{
    BBPM_QDEBUG("running: " << displayName());

    bool canContinue = true;
    foreach (ProjectExplorer::Task const& t, tasks_) {
        addTask(t);
        canContinue = false;
    }

    if (!canContinue) {
        emit addOutput(tr("Configuration is faulty. Check the Issues view for details.")
                     , BuildStep::MessageOutput);
        fi.reportResult(false);
        emit finished();
    } else {
        AbstractProcessStep::run(fi);
    }
}

BuildConfiguration* BuildStep::thisBuildConfiguration() const
{
    BuildConfiguration* bc = 0;
    if (ProjectExplorer::BuildConfiguration* bcBase = buildConfiguration()) {
        bc = qobject_cast<BuildConfiguration*>(bcBase);
    } else {
        // TODO: Do we need to do anything with this case?
        // From QmakeProjectManager:
        // That means the step is in the deploylist, so we listen to the active build
        // config changed signal and react...

        bcBase = target()->activeBuildConfiguration();
        bc = qobject_cast<BuildConfiguration*>(bcBase);
    }
    return bc;
}

ProjectExplorer::BuildStepConfigWidget* BuildStep::createConfigWidget()
{
    return new BuildStepConfigWidget(this);
}

bool BuildStep::immutable() const
{
    return false;
}

QVariantMap BuildStep::toMap() const
{
    QVariantMap map(ProjectExplorer::AbstractProcessStep::toMap());
    map.insert(QLatin1String(Constants::BS_KEY_ARGUMENTS), additionalArguments());
    return map;
}

bool BuildStep::fromMap(QVariantMap const& map)
{
    QString const args(map.value(QLatin1String(Constants::BS_KEY_ARGUMENTS)).toString());
    setAdditionalArguments(args);

    return ProjectExplorer::AbstractProcessStep::fromMap(map);
}

QString BuildStep::makeCommand(Utils::Environment const& env) const
{
    Q_UNUSED(env);
    return QLatin1String(Constants::COMMAND_BB2);
}

QString BuildStep::additionalArguments() const
{
    return Utils::QtcProcess::joinArgs(arguments_);
}

QString BuildStep::allArguments() const
{
    QStringList args(arguments_);

    // Collect implicit arguments not specified by user directly as option=value pair
    if (ProjectExplorer::BuildConfiguration* bc = buildConfiguration()) {
        QString const builddir(bc->buildDirectory().toString());
        if (!builddir.isEmpty())
            args.append(QLatin1String("--build-dir=") + builddir);
    }

    return Utils::QtcProcess::joinArgs(args);
}

void BuildStep::appendAdditionalArgument(QString const& arg)
{
    // TODO: use map or find duplicates?
    arguments_.append(arg);
}

void BuildStep::setAdditionalArguments(QString const& args)
{
    Utils::QtcProcess::SplitError err;
    QStringList argsList = Utils::QtcProcess::splitArgs(args, Utils::HostOsInfo::hostOs(), false, &err);
    if (err == Utils::QtcProcess::SplitOk) {
        arguments_ = argsList;
        emit argumentsChanged(args);
    }
}

ProjectExplorer::BuildConfiguration::BuildType
BuildStep::buildType() const
{
    // TODO: what is user inputs "variant = release" or mixed-case value?
    return arguments_.contains(QLatin1String("variant=release"))
            ? BuildConfiguration::Release
            : BuildConfiguration::Debug;
}

void
BuildStep::setBuildType(ProjectExplorer::BuildConfiguration::BuildType type)
{
    // TODO: Move literals to constants
    QString arg(QLatin1String("variant="));
    if (type == BuildConfiguration::Release)
        arg += QLatin1String("release");
    else
        arg += QLatin1String("debug");

    appendAdditionalArgument(arg);
}

BuildStepFactory::BuildStepFactory(QObject* parent)
    : IBuildStepFactory(parent)
{
}

/*static*/ BuildStepFactory* BuildStepFactory::getObject()
{
    return ExtensionSystem::PluginManager::getObject<BuildStepFactory>();
}

QList<Core::Id>
BuildStepFactory::availableCreationIds(ProjectExplorer::BuildStepList* parent) const
{
    return canHandle(parent)
        ? QList<Core::Id>() << Core::Id(Constants::BUILDSTEP_ID)
        : QList<Core::Id>();
}

QString BuildStepFactory::displayNameForId(Core::Id const id) const
{
    BBPM_QDEBUG("id: " << id.toString());

    QString name;
    if (id == Constants::BUILDSTEP_ID) {
        name = tr("Boost.Build"
                , "Display name for BoostBuildProjectManager::BuildStep id.");
    }
    return name;
}

bool BuildStepFactory::canCreate(ProjectExplorer::BuildStepList* parent
                               , Core::Id const id) const
{
    return canHandle(parent) && Core::Id(Constants::BUILDSTEP_ID) == id;
}

BuildStep*
BuildStepFactory::create(ProjectExplorer::BuildStepList* parent)
{
    ProjectExplorer::BuildStep* step = create(parent, Constants::BUILDSTEP_ID);
    return qobject_cast<BuildStep*>(step);
}

ProjectExplorer::BuildStep*
BuildStepFactory::create(ProjectExplorer::BuildStepList* parent, Core::Id const id)
{
    BBPM_QDEBUG("id: " << id.toString());
    if (!canCreate(parent, id))
        return 0;

    BuildStep* step = new BuildStep(parent);

    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
        step->appendAdditionalArgument(QLatin1String("--clean"));

    return step;
}

bool BuildStepFactory::canClone(ProjectExplorer::BuildStepList *parent
                              , ProjectExplorer::BuildStep *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::BuildStep*
BuildStepFactory::clone(ProjectExplorer::BuildStepList* parent
                      , ProjectExplorer::BuildStep* source)
{
    return canClone(parent, source)
            ? new BuildStep(parent, static_cast<BuildStep*>(source))
            : 0;
}

bool BuildStepFactory::canRestore(ProjectExplorer::BuildStepList* parent
                                , QVariantMap const& map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep*
BuildStepFactory::restore(ProjectExplorer::BuildStepList* parent
                        , QVariantMap const& map)
{
    Q_ASSERT(parent);

    if (canRestore(parent, map)) {
        QScopedPointer<BuildStep> bs(new BuildStep(parent));
        if (bs->fromMap(map))
            return bs.take();
    }
    return 0;
}

bool BuildStepFactory::canHandle(ProjectExplorer::BuildStepList* parent) const
{
    QTC_ASSERT(parent, return false);
    return parent->target()->project()->id() == Constants::PROJECT_ID;
}

BuildStepConfigWidget::BuildStepConfigWidget(BuildStep* step)
    : step_(step)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    arguments_ = new QLineEdit(this);
    arguments_->setText(step_->additionalArguments());
    fl->addRow(tr("Arguments:"), arguments_);

    updateDetails();

    connect(arguments_, SIGNAL(textChanged(QString))
          , step, SLOT(setAdditionalArguments(QString)));
    connect(step, SIGNAL(argumentsChanged(QString))
          , this, SLOT(updateDetails()));
    connect(step_->project(), SIGNAL(environmentChanged())
          , this, SLOT(updateDetails()));

    if (BuildConfiguration* bc = step_->thisBuildConfiguration()) {
        connect(bc, SIGNAL(buildDirectoryChanged())
              , this, SLOT(updateDetails()));
    }
}

BuildStepConfigWidget::~BuildStepConfigWidget()
{
}

QString BuildStepConfigWidget::displayName() const
{
    return tr(Constants::BOOSTBUILD
            , "BoostBuildProjectManager::BuildStep display name.");
}

QString BuildStepConfigWidget::summaryText() const
{
    return summaryText_;
}

void BuildStepConfigWidget::setSummaryText(QString const& text)
{
    if (text != summaryText_) {
        summaryText_ = text;
        emit updateSummary();
    }
}

void BuildStepConfigWidget::updateDetails()
{
    ProjectExplorer::ToolChain* tc
        = ProjectExplorer::ToolChainKitInformation::toolChain(step_->target()->kit());
    if (!tc) {
        setSummaryText(tr("<b>%1:</b> %2")
            .arg(QLatin1String(Constants::BOOSTBUILD))
            .arg(ProjectExplorer::ToolChainKitInformation::msgNoToolChainInTarget()));
        return;
    }

    BuildConfiguration* bc = step_->thisBuildConfiguration();
    QTC_ASSERT(bc, return;);

    ProjectExplorer::ProcessParameters params;
    params.setMacroExpander(bc->macroExpander());
    params.setEnvironment(bc->environment());
    params.setWorkingDirectory(bc->workingDirectory().toString());
    params.setCommand(step_->makeCommand(bc->environment()));
    params.setArguments(step_->allArguments());

    if (params.commandMissing()) {
        setSummaryText(tr("<b>%1:</b> %2 not found in the environment.")
            .arg(QLatin1String(Constants::BOOSTBUILD))
            .arg(params.command())); // Override display text
    } else {
        setSummaryText(params.summary(displayName()));
    }

}

} // namespace Internal
} // namespace BoostBuildProjectManager
