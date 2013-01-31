/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "makestep.h"
#include "ui_makestep.h"

#include "qt4project.h"
#include "qt4nodes.h"
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcprocess.h>
#include <qtsupport/qtparser.h>
#include <qtsupport/qtkitinformation.h>

#include <QDir>
#include <QFileInfo>

using ExtensionSystem::PluginManager;
using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
const char MAKESTEP_BS_ID[] = "Qt4ProjectManager.MakeStep";
const char MAKE_ARGUMENTS_KEY[] = "Qt4ProjectManager.MakeStep.MakeArguments";
const char MAKE_COMMAND_KEY[] = "Qt4ProjectManager.MakeStep.MakeCommand";
const char CLEAN_KEY[] = "Qt4ProjectManager.MakeStep.Clean";
}

MakeStep::MakeStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, Core::Id(MAKESTEP_BS_ID)),
    m_clean(false)
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

MakeStep::MakeStep(BuildStepList *bsl, const Core::Id id) :
    AbstractProcessStep(bsl, id),
    m_clean(false)
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
    QVariantMap map(AbstractProcessStep::toMap());
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

    return AbstractProcessStep::fromMap(map);
}

bool MakeStep::init()
{
    Qt4BuildConfiguration *bc = qt4BuildConfiguration();
    if (!bc)
        bc = qobject_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());

    m_tasks.clear();
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
    if (!tc) {
        m_tasks.append(Task(Task::Error, tr("Qt Creator needs a compiler set up to build. Configure a compiler in the kit options."),
                            Utils::FileName(), -1,
                            Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return true; // otherwise the tasks will not get reported
    }

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());

    QString workingDirectory;
    if (bc->subNodeBuild())
        workingDirectory = bc->subNodeBuild()->buildDir();
    else
        workingDirectory = bc->buildDirectory();
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
        if (tc->targetAbi().binaryFormat() != Abi::PEFormat )
            Utils::QtcProcess::addArg(&args, QLatin1String("-w"));
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
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    m_scriptTarget = (static_cast<Qt4Project *>(bc->target()->project())->rootQt4ProjectNode()->projectType() == ScriptTemplate);

    return AbstractProcessStep::init();
}

void MakeStep::run(QFutureInterface<bool> & fi)
{
    bool canContinue = true;
    foreach (const Task &t, m_tasks) {
        addTask(t);
        canContinue = false;
    }
    if (!canContinue) {
        emit addOutput(tr("Configuration is faulty. Check the Issues view for details."), BuildStep::MessageOutput);
        fi.reportResult(false);
        return;
    }

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
    ToolChain *tc
            = ToolChainKitInformation::toolChain(m_makeStep->target()->kit());
    Qt4BuildConfiguration *bc = m_makeStep->qt4BuildConfiguration();
    if (!bc)
        bc = qobject_cast<Qt4BuildConfiguration *>(m_makeStep->target()->activeBuildConfiguration());

    if (tc && bc)
        m_ui->makeLabel->setText(tr("Override %1:").arg(tc->makeCommand(bc->environment())));
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
    param.setWorkingDirectory(bc->buildDirectory());
    QString makeCmd = tc->makeCommand(bc->environment());
    if (!m_makeStep->makeCommand().isEmpty())
        makeCmd = m_makeStep->makeCommand();
    param.setCommand(makeCmd);

    QString args = m_makeStep->userArguments();

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
        if (tc->targetAbi().binaryFormat() != Abi::PEFormat )
            Utils::QtcProcess::addArg(&args, QLatin1String("-w"));
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

bool MakeStepFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    if (parent->target()->project()->id() == Constants::QT4PROJECT_ID)
        return id == MAKESTEP_BS_ID;
    return false;
}

BuildStep *MakeStepFactory::create(BuildStepList *parent, const Core::Id id)
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
    if (parent->target()->project()->id() == Constants::QT4PROJECT_ID)
        return QList<Core::Id>() << Core::Id(MAKESTEP_BS_ID);
    return QList<Core::Id>();
}

QString MakeStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == MAKESTEP_BS_ID)
        return tr("Make");
    return QString();
}
