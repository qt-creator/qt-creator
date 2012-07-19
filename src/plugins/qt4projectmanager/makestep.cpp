/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "makestep.h"
#include "ui_makestep.h"

#include "qt4project.h"
#include "qt4nodes.h"
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/profileinformation.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcprocess.h>
#include <qtsupport/qtparser.h>
#include <qtsupport/qtprofileinformation.h>

#include <QDir>
#include <QFileInfo>

using ExtensionSystem::PluginManager;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
const char * const MAKESTEP_BS_ID("Qt4ProjectManager.MakeStep");

const char * const MAKE_ARGUMENTS_KEY("Qt4ProjectManager.MakeStep.MakeArguments");
const char * const MAKE_COMMAND_KEY("Qt4ProjectManager.MakeStep.MakeCommand");
const char * const CLEAN_KEY("Qt4ProjectManager.MakeStep.Clean");
}

MakeStep::MakeStep(ProjectExplorer::BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(MAKESTEP_BS_ID)),
    m_clean(false)
{
    ctor();
}

MakeStep::MakeStep(ProjectExplorer::BuildStepList *bsl, MakeStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_clean(bs->m_clean),
    m_userArgs(bs->m_userArgs),
    m_makeCmd(bs->m_makeCmd)
{
    ctor();
}

MakeStep::MakeStep(ProjectExplorer::BuildStepList *bsl, const Core::Id id) :
    AbstractProcessStep(bsl, id),
    m_clean(false)
{
    ctor();
}

void MakeStep::ctor()
{
    setDefaultDisplayName(tr("Make", "Qt4 MakeStep display name."));
}

void MakeStep::setMakeCommand(const QString &make)
{
    m_makeCmd = make;
}

MakeStep::~MakeStep()
{
}

Qt4BuildConfiguration *MakeStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

void MakeStep::setClean(bool clean)
{
    m_clean = clean;
}

bool MakeStep::isClean() const
{
    return m_clean;
}

QString MakeStep::makeCommand() const
{
    return m_makeCmd;
}

QVariantMap MakeStep::toMap() const
{
    QVariantMap map(ProjectExplorer::AbstractProcessStep::toMap());
    map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), m_userArgs);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), m_makeCmd);
    map.insert(QLatin1String(CLEAN_KEY), m_clean);
    return map;
}

bool MakeStep::fromMap(const QVariantMap &map)
{
    m_makeCmd = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();
    m_userArgs = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
    m_clean = map.value(QLatin1String(CLEAN_KEY)).toBool();

    return ProjectExplorer::AbstractProcessStep::fromMap(map);
}

bool MakeStep::init()
{
    Qt4BuildConfiguration *bc = qt4BuildConfiguration();
    if (!bc)
        bc = qobject_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());

    m_tasks.clear();
    if (!bc) {
        m_tasks.append(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                             tr("Qt Creator needs a build configuration set up to build. Configure a tool chain in Project mode."),
                                             Utils::FileName(), -1,
                                             Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return false;
    }

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(target()->profile());
    if (!tc) {
        m_tasks.append(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                             tr("Qt Creator needs a tool chain set up to build. Configure a tool chain the profile options."),
                                             Utils::FileName(), -1,
                                             Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return false;
    }

    ProjectExplorer::ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());

    QString workingDirectory;
    if (bc->subNodeBuild())
        workingDirectory = bc->subNodeBuild()->buildDir();
    else
        workingDirectory = bc->buildDirectory();
    pp->setWorkingDirectory(workingDirectory);

    QString makeCmd = tc->makeCommand();
    if (!m_makeCmd.isEmpty())
        makeCmd = m_makeCmd;
    pp->setCommand(makeCmd);

    // If we are cleaning, then make can fail with a error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on a already clean project
    setIgnoreReturnValue(m_clean);

    QString args;

    Qt4ProjectManager::Qt4ProFileNode *subNode = bc->subNodeBuild();
    if (subNode) {
        QString makefile = subNode->makefile();
        if (makefile.isEmpty())
            makefile = QLatin1String("Makefile");
        if (subNode->isDebugAndRelease()) {
            if (bc->buildType() == Qt4BuildConfiguration::Debug)
                makefile += QLatin1String(".Debug");
            else
                makefile += QLatin1String(".Release");
        }
        if (makefile != QLatin1String("Makefile")) {
            Utils::QtcProcess::addArg(&args, QLatin1String("-f"));
            Utils::QtcProcess::addArg(&args, makefile);
        }
        m_makeFileToCheck = QDir(workingDirectory).filePath(makefile);
    } else {
        if (!bc->makefile().isEmpty()) {
            Utils::QtcProcess::addArg(&args, QLatin1String("-f"));
            Utils::QtcProcess::addArg(&args, bc->makefile());
            m_makeFileToCheck = QDir(workingDirectory).filePath(bc->makefile());
        } else {
            m_makeFileToCheck = QDir(workingDirectory).filePath(QLatin1String("Makefile"));
        }
    }

    Utils::QtcProcess::addArgs(&args, m_userArgs);

    if (!isClean()) {
        if (!bc->defaultMakeTarget().isEmpty())
            Utils::QtcProcess::addArg(&args, bc->defaultMakeTarget());
    }

    if (bc->fileNodeBuild() && subNode) {
        QString objectsDir = subNode->objectsDirectory();
        if (objectsDir.isEmpty()) {
            objectsDir = subNode->buildDir(bc);
            if (subNode->isDebugAndRelease()) {
                if (bc->buildType() == Qt4BuildConfiguration::Debug)
                    objectsDir += QLatin1String("/debug");
                else
                    objectsDir += QLatin1String("/release");
            }
        }
        QString relObjectsDir = QDir(pp->workingDirectory()).relativeFilePath(objectsDir);
        if (!relObjectsDir.isEmpty())
            relObjectsDir += QLatin1Char('/');
        QString objectFile = relObjectsDir +
                QFileInfo(bc->fileNodeBuild()->path()).baseName() +
                subNode->objectExtension();
        Utils::QtcProcess::addArg(&args, objectFile);
    }
    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    // -w option enables "Enter"/"Leaving directory" messages, which we need for detecting the
    // absolute file path
    // doing this without the user having a way to override this is rather bad
    // so we only do it for unix and if the user didn't override the make command
    // but for now this is the least invasive change
    // We also prepend "L" to the MAKEFLAGS, so that nmake / jom are less verbose
    if (tc && m_makeCmd.isEmpty()) {
        if (tc->targetAbi().binaryFormat() != ProjectExplorer::Abi::PEFormat )
            Utils::QtcProcess::addArg(&args, QLatin1String("-w"));
        if (tc->targetAbi().os() == ProjectExplorer::Abi::WindowsOS
                && tc->targetAbi().osFlavor() != ProjectExplorer::Abi::WindowsMSysFlavor) {
            const QString makeFlags = QLatin1String("MAKEFLAGS");
            env.set(makeFlags, QLatin1Char('L') + env.value(makeFlags));
        }
    }

    pp->setEnvironment(env);
    pp->setArguments(args);

    ProjectExplorer::IOutputParser *parser = 0;
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target()->profile());
    if (version)
        parser = version->createOutputParser();
    if (parser)
        parser->appendOutputParser(new QtSupport::QtParser);
    else
        parser = new QtSupport::QtParser;
    if (tc)
        parser->appendOutputParser(tc->outputParser());

    parser->setWorkingDirectory(workingDirectory);

    setOutputParser(parser);

    m_scriptTarget = (static_cast<Qt4Project *>(bc->target()->project())->rootQt4ProjectNode()->projectType() == ScriptTemplate);

    return AbstractProcessStep::init();
}

void MakeStep::run(QFutureInterface<bool> & fi)
{
    if (m_scriptTarget) {
        fi.reportResult(true);
        return;
    }

    if (!QFileInfo(m_makeFileToCheck).exists()) {
        if (!ignoreReturnValue())
            emit addOutput(tr("Cannot find Makefile. Check your build settings."), BuildStep::MessageOutput);
        fi.reportResult(ignoreReturnValue());
        return;
    }

    // Warn on common error conditions:
    bool canContinue = true;
    foreach (const ProjectExplorer::Task &t, m_tasks) {
        addTask(t);
        if (t.type == ProjectExplorer::Task::Error)
            canContinue = false;
    }
    if (!canContinue) {
        emit addOutput(tr("Configuration is faulty. Check the Issues view for details."), BuildStep::MessageOutput);
        fi.reportResult(false);
        return;
    }

    AbstractProcessStep::run(fi);
}

bool MakeStep::processSucceeded(int exitCode, QProcess::ExitStatus status)
{
    // Symbian does retun 0, even on failed makes! So we check for fatal make errors here.
    if (outputParser() && outputParser()->hasFatalErrors())
        return false;

    return AbstractProcessStep::processSucceeded(exitCode, status);
}

bool MakeStep::immutable() const
{
    return false;
}

ProjectExplorer::BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeStepConfigWidget(this);
}

QString MakeStep::userArguments()
{
    return m_userArgs;
}

void MakeStep::setUserArguments(const QString &arguments)
{
    m_userArgs = arguments;
    emit userArgumentsChanged();
}

MakeStepConfigWidget::MakeStepConfigWidget(MakeStep *makeStep)
    : BuildStepConfigWidget(), m_ui(new Internal::Ui::MakeStep), m_makeStep(makeStep), m_bc(0), m_ignoreChange(false)
{
    m_ui->setupUi(this);

    m_ui->makePathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->makePathChooser->setBaseDirectory(Utils::PathChooser::homePath());


    const QString &makeCmd = m_makeStep->makeCommand();
    m_ui->makePathChooser->setPath(makeCmd);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->userArguments());

    updateDetails();

    connect(m_ui->makePathChooser, SIGNAL(changed(QString)),
            this, SLOT(makeEdited()));
    connect(m_ui->makeArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeArgumentsLineEdited()));

    connect(makeStep, SIGNAL(userArgumentsChanged()),
            this, SLOT(userArgumentsChanged()));

    ProjectExplorer::BuildConfiguration *bc = makeStep->buildConfiguration();
    if (!bc) {
        // That means the step is in the deploylist, so we listen to the active build config
        // changed signal and react to the buildDirectoryChanged() signal of the buildconfiguration
        bc = makeStep->target()->activeBuildConfiguration();
        m_bc = bc;
        connect (makeStep->target(), SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                 this, SLOT(activeBuildConfigurationChanged()));
    }

    if (bc) {
        connect(bc, SIGNAL(buildDirectoryChanged()),
                this, SLOT(updateDetails()));
    }

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
    connect(m_makeStep->target(), SIGNAL(profileChanged()), this, SLOT(updateDetails()));
}

void MakeStepConfigWidget::activeBuildConfigurationChanged()
{
    if (m_bc) {
        disconnect(m_bc, SIGNAL(buildDirectoryChanged()),
                this, SLOT(updateDetails()));
    }

    m_bc = m_makeStep->target()->activeBuildConfiguration();
    updateDetails();

    if (m_bc) {
        connect(m_bc, SIGNAL(buildDirectoryChanged()),
                this, SLOT(updateDetails()));
    }
}

void MakeStepConfigWidget::setSummaryText(const QString &text)
{
    if (text == m_summaryText)
        return;
    m_summaryText = text;
    emit updateSummary();
}

MakeStepConfigWidget::~MakeStepConfigWidget()
{
    delete m_ui;
}

void MakeStepConfigWidget::updateDetails()
{
    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainProfileInformation::toolChain(m_makeStep->target()->profile());
    if (tc)
        m_ui->makeLabel->setText(tr("Override %1:").arg(tc->makeCommand()));
    else
        m_ui->makeLabel->setText(tr("Make:"));

    if (!tc) {
        setSummaryText(tr("<b>Make:</b> No tool chain set in profile."));
        return;
    }
    Qt4BuildConfiguration *bc = m_makeStep->qt4BuildConfiguration();
    if (!bc)
        bc = qobject_cast<Qt4BuildConfiguration *>(m_makeStep->target()->activeBuildConfiguration());
    if (!bc) {
        setSummaryText(tr("<b>Make:</b> No Qt4 build configuration."));
        return;
    }

    ProjectExplorer::ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory());
    QString makeCmd = tc->makeCommand();
    if (!m_makeStep->makeCommand().isEmpty())
        makeCmd = m_makeStep->makeCommand();
    param.setCommand(makeCmd);

    QString args = m_makeStep->userArguments();
    if (!m_makeStep->isClean()) {
        if (!bc->defaultMakeTarget().isEmpty())
            Utils::QtcProcess::addArg(&args, bc->defaultMakeTarget());
    }

    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    // -w option enables "Enter"/"Leaving directory" messages, which we need for detecting the
    // absolute file path
    // FIXME doing this without the user having a way to override this is rather bad
    // so we only do it for unix and if the user didn't override the make command
    // but for now this is the least invasive change
    // We also prepend "L" to the MAKEFLAGS, so that nmake / jom are less verbose
    if (tc && m_makeStep->makeCommand().isEmpty()) {
        if (tc->targetAbi().binaryFormat() != ProjectExplorer::Abi::PEFormat )
            Utils::QtcProcess::addArg(&args, QLatin1String("-w"));
        if (tc->targetAbi().os() == ProjectExplorer::Abi::WindowsOS
                && tc->targetAbi().osFlavor() != ProjectExplorer::Abi::WindowsMSysFlavor) {
            const QString makeFlags = QLatin1String("MAKEFLAGS");
            env.set(makeFlags, QLatin1Char('L') + env.value(makeFlags));
        }
    }
    param.setArguments(args);
    param.setEnvironment(env);

    if (param.commandMissing())
        setSummaryText(tr("<b>Make:</b> %1 not found in the environment.").arg(makeCmd)); // Override display text
    else
        setSummaryText(param.summaryInWorkdir(displayName()));
}

QString MakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

QString MakeStepConfigWidget::displayName() const
{
    return m_makeStep->displayName();
}

void MakeStepConfigWidget::userArgumentsChanged()
{
    if (m_ignoreChange)
        return;
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->userArguments());
    updateDetails();
}

void MakeStepConfigWidget::makeEdited()
{
    m_makeStep->setMakeCommand(m_ui->makePathChooser->rawPath());
    updateDetails();
}

void MakeStepConfigWidget::makeArgumentsLineEdited()
{
    m_ignoreChange = true;
    m_makeStep->setUserArguments(m_ui->makeArgumentsLineEdit->text());
    m_ignoreChange = false;
    updateDetails();
}

///
// MakeStepFactory
///

MakeStepFactory::MakeStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

MakeStepFactory::~MakeStepFactory()
{
}

bool MakeStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    if (parent->target()->project()->id() != Core::Id(Constants::QT4PROJECT_ID))
        return false;
    return (id == Core::Id(MAKESTEP_BS_ID));
}

ProjectExplorer::BuildStep *MakeStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    MakeStep *step = new MakeStep(parent);
    if (parent->id() == Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN)) {
        step->setClean(true);
        step->setUserArguments(QLatin1String("clean"));
    }
    return step;
}

bool MakeStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::BuildStep *MakeStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new MakeStep(parent, static_cast<MakeStep *>(source));
}

bool MakeStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *MakeStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    MakeStep *bs(new MakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Core::Id> MakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->target()->project()->id() == Core::Id(Constants::QT4PROJECT_ID))
        return QList<Core::Id>() << Core::Id(MAKESTEP_BS_ID);
    return QList<Core::Id>();
}

QString MakeStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(MAKESTEP_BS_ID))
        return tr("Make");
    return QString();
}
