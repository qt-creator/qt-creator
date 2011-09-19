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

#include "makestep.h"
#include "ui_makestep.h"

#include "qt4project.h"
#include "qt4target.h"
#include "qt4nodes.h"
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcprocess.h>
#include <qtsupport/qtparser.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

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
    AbstractProcessStep(bsl, QLatin1String(MAKESTEP_BS_ID)),
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

MakeStep::MakeStep(ProjectExplorer::BuildStepList *bsl, const QString &id) :
    AbstractProcessStep(bsl, id),
    m_clean(false)
{
    ctor();
}

void MakeStep::ctor()
{
    setDefaultDisplayName(tr("Make", "Qt4 MakeStep display name."));
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

    m_tasks.clear();
    if (!bc->toolChain()) {
        m_tasks.append(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                             tr("Qt Creator needs a tool chain set up to build. Configure a tool chain in Project mode."),
                                             QString(), -1,
                                             QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    ProjectExplorer::ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());

    Utils::Environment environment = bc->environment();
    pp->setEnvironment(environment);

    QString workingDirectory;
    if (bc->subNodeBuild())
        workingDirectory = bc->subNodeBuild()->buildDir();
    else
        workingDirectory = bc->buildDirectory();
    pp->setWorkingDirectory(workingDirectory);

    QString makeCmd = bc->makeCommand();
    if (!m_makeCmd.isEmpty())
        makeCmd = m_makeCmd;
    pp->setCommand(makeCmd);

    // If we are cleaning, then make can fail with a error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on a already clean project
    setIgnoreReturnValue(m_clean);

    QString args;

    ProjectExplorer::ToolChain *toolchain = bc->toolChain();

    if (bc->subNodeBuild()) {
        QString makefile = bc->subNodeBuild()->makefile();
        if (!makefile.isEmpty()) {
            Utils::QtcProcess::addArg(&args, QLatin1String("-f"));
            Utils::QtcProcess::addArg(&args, makefile);
            m_makeFileToCheck = QDir(workingDirectory).filePath(makefile);
        } else {
            m_makeFileToCheck = QDir(workingDirectory).filePath("Makefile");
        }
    } else {
        if (!bc->makefile().isEmpty()) {
            Utils::QtcProcess::addArg(&args, QLatin1String("-f"));
            Utils::QtcProcess::addArg(&args, bc->makefile());
            m_makeFileToCheck = QDir(workingDirectory).filePath(bc->makefile());
        } else {
            m_makeFileToCheck = QDir(workingDirectory).filePath("Makefile");
        }
    }

    Utils::QtcProcess::addArgs(&args, m_userArgs);

    if (!isClean()) {
        if (!bc->defaultMakeTarget().isEmpty())
            Utils::QtcProcess::addArg(&args, bc->defaultMakeTarget());
    }
    // -w option enables "Enter"/"Leaving directory" messages, which we need for detecting the
    // absolute file path
    // FIXME doing this without the user having a way to override this is rather bad
    // so we only do it for unix and if the user didn't override the make command
    // but for now this is the least invasive change
    if (toolchain
            && toolchain->targetAbi().binaryFormat() != ProjectExplorer::Abi::PEFormat
            && m_makeCmd.isEmpty())
        Utils::QtcProcess::addArg(&args, QLatin1String("-w"));

    setEnabled(true);
    pp->setArguments(args);

    ProjectExplorer::IOutputParser *parser = 0;
    if (bc->qtVersion())
        parser = bc->qtVersion()->createOutputParser();
    if (parser)
        parser->appendOutputParser(new QtSupport::QtParser);
    else
        parser = new QtSupport::QtParser;
    if (toolchain)
        parser->appendOutputParser(toolchain->outputParser());

    parser->setWorkingDirectory(workingDirectory);

    setOutputParser(parser);

    return AbstractProcessStep::init();
}

void MakeStep::run(QFutureInterface<bool> & fi)
{
    if (qt4BuildConfiguration()->qt4Target()->qt4Project()->rootQt4ProjectNode()->projectType() == ScriptTemplate) {
        fi.reportResult(true);
        return;
    }

    if (!QFileInfo(m_makeFileToCheck).exists()) {
        if (!m_clean)
            emit addOutput(tr("Cannot find Makefile. Check your build settings."), BuildStep::MessageOutput);
        fi.reportResult(m_clean);
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
        emit addOutput(tr("Configuration is faulty. Check the Build Issues view for details."), BuildStep::MessageOutput);
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
    : BuildStepConfigWidget(), m_ui(new Ui::MakeStep), m_makeStep(makeStep), m_ignoreChange(false)
{
    m_ui->setupUi(this);

    m_ui->makePathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->makePathChooser->setBaseDirectory(Utils::PathChooser::homePath());


    const QString &makeCmd = m_makeStep->m_makeCmd;
    m_ui->makePathChooser->setPath(makeCmd);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->userArguments());

    updateMakeOverrideLabel();
    updateDetails();

    connect(m_ui->makePathChooser, SIGNAL(changed(QString)),
            this, SLOT(makeEdited()));
    connect(m_ui->makeArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeArgumentsLineEdited()));

    connect(makeStep, SIGNAL(userArgumentsChanged()),
            this, SLOT(userArgumentsChanged()));
    connect(makeStep->buildConfiguration(), SIGNAL(buildDirectoryChanged()),
            this, SLOT(updateDetails()));
    connect(makeStep->buildConfiguration(), SIGNAL(toolChainChanged()),
            this, SLOT(updateDetails()));

    connect(makeStep->qt4BuildConfiguration(), SIGNAL(qtVersionChanged()),
            this, SLOT(qtVersionChanged()));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrideLabel()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
}

MakeStepConfigWidget::~MakeStepConfigWidget()
{
    delete m_ui;
}

void MakeStepConfigWidget::qtVersionChanged()
{
    updateMakeOverrideLabel();
    updateDetails();
}

void MakeStepConfigWidget::updateMakeOverrideLabel()
{
    Qt4BuildConfiguration *qt4bc = m_makeStep->qt4BuildConfiguration();
    m_ui->makeLabel->setText(tr("Override %1:").arg(qt4bc->makeCommand()));
}

void MakeStepConfigWidget::updateDetails()
{
    Qt4BuildConfiguration *bc = m_makeStep->qt4BuildConfiguration();

    ProjectExplorer::ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory());
    param.setEnvironment(bc->environment());
    QString makeCmd = bc->makeCommand();
    if (!m_makeStep->m_makeCmd.isEmpty())
        makeCmd = m_makeStep->m_makeCmd;
    param.setCommand(makeCmd);
    if (param.commandMissing()) {
        m_summaryText = tr("<b>Make:</b> %1 not found in the environment.").arg(makeCmd);
        emit updateSummary();
        return;
    }

    QString args = m_makeStep->userArguments();
    if (!m_makeStep->isClean()) {
        if (!bc->defaultMakeTarget().isEmpty())
            Utils::QtcProcess::addArg(&args, bc->defaultMakeTarget());
    }
    // -w option enables "Enter"/"Leaving directory" messages, which we need for detecting the
    // absolute file path
    // FIXME doing this without the user having a way to override this is rather bad
    // so we only do it for unix and if the user didn't override the make command
    // but for now this is the least invasive change
    ProjectExplorer::ToolChain *toolChain = bc->toolChain();
    if (toolChain
        && toolChain->targetAbi().binaryFormat() != ProjectExplorer::Abi::PEFormat
        && m_makeStep->m_makeCmd.isEmpty())
        Utils::QtcProcess::addArg(&args, QLatin1String("-w"));
    param.setArguments(args);
    m_summaryText = param.summaryInWorkdir(displayName());
    emit updateSummary();
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
    m_makeStep->m_makeCmd = m_ui->makePathChooser->rawPath();
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

bool MakeStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const QString &id) const
{
    if (parent->target()->project()->id() != QLatin1String(Constants::QT4PROJECT_ID))
        return false;
    return (id == QLatin1String(MAKESTEP_BS_ID));
}

ProjectExplorer::BuildStep *MakeStepFactory::create(ProjectExplorer::BuildStepList *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new MakeStep(parent);
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
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
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

QStringList MakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->target()->project()->id() == QLatin1String(Constants::QT4PROJECT_ID))
        return QStringList() << QLatin1String(MAKESTEP_BS_ID);
    return QStringList();
}

QString MakeStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(MAKESTEP_BS_ID))
        return tr("Make");
    return QString();
}
