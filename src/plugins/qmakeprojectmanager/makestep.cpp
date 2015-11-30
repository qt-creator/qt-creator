/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "makestep.h"
#include "ui_makestep.h"

#include "qmakeparser.h"
#include "qmakeproject.h"
#include "qmakenodes.h"
#include "qmakebuildconfiguration.h"
#include "qmakeprojectmanagerconstants.h"

#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/xcodebuildparser.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QFileInfo>

using ExtensionSystem::PluginManager;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;

namespace {
const char MAKESTEP_BS_ID[] = "Qt4ProjectManager.MakeStep";
const char MAKE_ARGUMENTS_KEY[] = "Qt4ProjectManager.MakeStep.MakeArguments";
const char AUTOMATICLY_ADDED_MAKE_ARGUMENTS_KEY[] = "Qt4ProjectManager.MakeStep.AutomaticallyAddedMakeArguments";
const char MAKE_COMMAND_KEY[] = "Qt4ProjectManager.MakeStep.MakeCommand";
const char CLEAN_KEY[] = "Qt4ProjectManager.MakeStep.Clean";
}

MakeStep::MakeStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(MAKESTEP_BS_ID))
{
    ctor();
}

MakeStep::MakeStep(BuildStepList *bsl, MakeStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_clean(bs->m_clean),
    m_userArgs(bs->m_userArgs),
    m_makeCmd(bs->m_makeCmd)
{
    ctor();
}

MakeStep::MakeStep(BuildStepList *bsl, Core::Id id) :
    AbstractProcessStep(bsl, id)
{
    ctor();
}

void MakeStep::ctor()
{
    setDefaultDisplayName(tr("Make", "Qt MakeStep display name."));
}

void MakeStep::setMakeCommand(const QString &make)
{
    m_makeCmd = make;
}

QmakeBuildConfiguration *MakeStep::qmakeBuildConfiguration() const
{
    return static_cast<QmakeBuildConfiguration *>(buildConfiguration());
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
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), m_userArgs);
    map.insert(QLatin1String(MAKE_COMMAND_KEY), m_makeCmd);
    map.insert(QLatin1String(CLEAN_KEY), m_clean);
    map.insert(QLatin1String(AUTOMATICLY_ADDED_MAKE_ARGUMENTS_KEY), automaticallyAddedArguments());
    return map;
}

QStringList MakeStep::automaticallyAddedArguments() const
{
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
    if (!tc || tc->targetAbi().binaryFormat() == Abi::PEFormat)
        return QStringList();
    return QStringList() << QLatin1String("-w") << QLatin1String("-r");
}

bool MakeStep::fromMap(const QVariantMap &map)
{
    m_makeCmd = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();
    m_userArgs = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
    m_clean = map.value(QLatin1String(CLEAN_KEY)).toBool();
    QStringList oldAddedArgs
            = map.value(QLatin1String(AUTOMATICLY_ADDED_MAKE_ARGUMENTS_KEY)).toStringList();
    foreach (const QString &newArg, automaticallyAddedArguments()) {
        if (oldAddedArgs.contains(newArg))
            continue;
        m_userArgs.prepend(newArg + QLatin1Char(' '));
    }

    return AbstractProcessStep::fromMap(map);
}

bool MakeStep::init(QList<const BuildStep *> &earlierSteps)
{
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    if (!bc)
        bc = qobject_cast<QmakeBuildConfiguration *>(target()->activeBuildConfiguration());
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());

    QString workingDirectory;
    if (bc->subNodeBuild())
        workingDirectory = bc->subNodeBuild()->buildDir();
    else
        workingDirectory = bc->buildDirectory().toString();
    pp->setWorkingDirectory(workingDirectory);

    QString makeCmd = tc->makeCommand(bc->environment());
    if (!m_makeCmd.isEmpty())
        makeCmd = m_makeCmd;
    pp->setCommand(makeCmd);

    // If we are cleaning, then make can fail with a error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on a already clean project
    setIgnoreReturnValue(m_clean);

    QString args;

    QmakeProjectManager::QmakeProFileNode *subNode = bc->subNodeBuild();
    if (subNode) {
        QString makefile = subNode->makefile();
        if (makefile.isEmpty())
            makefile = QLatin1String("Makefile");
        // Use Makefile.Debug and Makefile.Release
        // for file builds, since the rules for that are
        // only in those files.
        if (subNode->isDebugAndRelease() && bc->fileNodeBuild()) {
            if (bc->buildType() == QmakeBuildConfiguration::Debug)
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
    if (bc->fileNodeBuild() && subNode) {
        QString objectsDir = subNode->objectsDirectory();
        if (objectsDir.isEmpty()) {
            objectsDir = subNode->buildDir(bc);
            if (subNode->isDebugAndRelease()) {
                if (bc->buildType() == QmakeBuildConfiguration::Debug)
                    objectsDir += QLatin1String("/debug");
                else
                    objectsDir += QLatin1String("/release");
            }
        }
        QString relObjectsDir = QDir(pp->workingDirectory()).relativeFilePath(objectsDir);
        if (relObjectsDir == QLatin1String("."))
            relObjectsDir.clear();
        if (!relObjectsDir.isEmpty())
            relObjectsDir += QLatin1Char('/');
        QString objectFile = relObjectsDir +
                bc->fileNodeBuild()->filePath().toFileInfo().baseName() +
                subNode->objectExtension();
        Utils::QtcProcess::addArg(&args, objectFile);
    }
    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    // We also prepend "L" to the MAKEFLAGS, so that nmake / jom are less verbose
    if (tc && m_makeCmd.isEmpty()) {
        if (tc->targetAbi().os() == Abi::WindowsOS
                && tc->targetAbi().osFlavor() != Abi::WindowsMSysFlavor) {
            const QString makeFlags = QLatin1String("MAKEFLAGS");
            env.set(makeFlags, QLatin1Char('L') + env.value(makeFlags));
        }
    }

    pp->setEnvironment(env);
    pp->setArguments(args);
    pp->resolveAll();

    setOutputParser(new ProjectExplorer::GnuMakeParser());
    if (tc && tc->targetAbi().os() == Abi::MacOS)
        appendOutputParser(new XcodebuildParser);
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());
    appendOutputParser(new QMakeParser); // make may cause qmake to be run, add last to make sure
                                         // it has a low priority.

    m_scriptTarget = (static_cast<QmakeProject *>(bc->target()->project())->rootQmakeProjectNode()->projectType() == ScriptTemplate);

    return AbstractProcessStep::init(earlierSteps);
}

void MakeStep::run(QFutureInterface<bool> & fi)
{
    if (m_scriptTarget) {
        fi.reportResult(true);
        emit finished();
        return;
    }

    if (!QFileInfo::exists(m_makeFileToCheck)) {
        if (!ignoreReturnValue())
            emit addOutput(tr("Cannot find Makefile. Check your build settings."), BuildStep::MessageOutput);
        fi.reportResult(ignoreReturnValue());
        emit finished();
        return;
    }

    AbstractProcessStep::run(fi);
}

bool MakeStep::immutable() const
{
    return false;
}

BuildStepConfigWidget *MakeStep::createConfigWidget()
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
    : BuildStepConfigWidget(), m_ui(new Internal::Ui::MakeStep), m_makeStep(makeStep)
{
    m_ui->setupUi(this);

    m_ui->makePathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->makePathChooser->setBaseDirectory(Utils::PathChooser::homePath());
    m_ui->makePathChooser->setHistoryCompleter(QLatin1String("PE.MakeCommand.History"));

    const QString &makeCmd = m_makeStep->makeCommand();
    m_ui->makePathChooser->setPath(makeCmd);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->userArguments());

    updateDetails();

    connect(m_ui->makePathChooser, SIGNAL(rawPathChanged(QString)),
            this, SLOT(makeEdited()));
    connect(m_ui->makeArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeArgumentsLineEdited()));

    connect(makeStep, SIGNAL(userArgumentsChanged()),
            this, SLOT(userArgumentsChanged()));

    BuildConfiguration *bc = makeStep->buildConfiguration();
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
        connect(bc, &BuildConfiguration::environmentChanged,
                this, &MakeStepConfigWidget::updateDetails);
    }

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
    connect(m_makeStep->target(), SIGNAL(kitChanged()), this, SLOT(updateDetails()));
}

void MakeStepConfigWidget::activeBuildConfigurationChanged()
{
    if (m_bc) {
        disconnect(m_bc, SIGNAL(buildDirectoryChanged()),
                this, SLOT(updateDetails()));
        disconnect(m_bc, &BuildConfiguration::environmentChanged,
                   this, &MakeStepConfigWidget::updateDetails);
    }

    m_bc = m_makeStep->target()->activeBuildConfiguration();
    updateDetails();

    if (m_bc) {
        connect(m_bc, SIGNAL(buildDirectoryChanged()),
                this, SLOT(updateDetails()));
        connect(m_bc, &BuildConfiguration::environmentChanged,
                this, &MakeStepConfigWidget::updateDetails);
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
    ToolChain *tc
            = ToolChainKitInformation::toolChain(m_makeStep->target()->kit());
    QmakeBuildConfiguration *bc = m_makeStep->qmakeBuildConfiguration();
    if (!bc)
        bc = qobject_cast<QmakeBuildConfiguration *>(m_makeStep->target()->activeBuildConfiguration());

    if (tc && bc)
        m_ui->makeLabel->setText(tr("Override %1:").arg(QDir::toNativeSeparators(tc->makeCommand(bc->environment()))));
    else
        m_ui->makeLabel->setText(tr("Make:"));

    if (!tc) {
        setSummaryText(tr("<b>Make:</b> %1").arg(ProjectExplorer::ToolChainKitInformation::msgNoToolChainInTarget()));
        return;
    }
    if (!bc) {
        setSummaryText(tr("<b>Make:</b> No Qt build configuration."));
        return;
    }

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    QString makeCmd = tc->makeCommand(bc->environment());
    if (!m_makeStep->makeCommand().isEmpty())
        makeCmd = m_makeStep->makeCommand();
    param.setCommand(makeCmd);

    QString args = m_makeStep->userArguments();

    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
    // We prepend "L" to the MAKEFLAGS, so that nmake / jom are less verbose
    // FIXME doing this without the user having a way to override this is rather bad
    if (tc && m_makeStep->makeCommand().isEmpty()) {
        if (tc->targetAbi().os() == Abi::WindowsOS
                && tc->targetAbi().osFlavor() != Abi::WindowsMSysFlavor) {
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
    IBuildStepFactory(parent)
{
}

MakeStepFactory::~MakeStepFactory()
{
}

bool MakeStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    if (parent->target()->project()->id() == Constants::QMAKEPROJECT_ID)
        return id == MAKESTEP_BS_ID;
    return false;
}

BuildStep *MakeStepFactory::create(BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    MakeStep *step = new MakeStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setClean(true);
        step->setUserArguments(QLatin1String("clean"));
    }
    return step;
}

bool MakeStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *MakeStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new MakeStep(parent, static_cast<MakeStep *>(source));
}

bool MakeStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *MakeStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    MakeStep *bs(new MakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Core::Id> MakeStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->target()->project()->id() == Constants::QMAKEPROJECT_ID)
        return QList<Core::Id>() << Core::Id(MAKESTEP_BS_ID);
    return QList<Core::Id>();
}

QString MakeStepFactory::displayNameForId(Core::Id id) const
{
    if (id == MAKESTEP_BS_ID)
        return tr("Make");
    return QString();
}
