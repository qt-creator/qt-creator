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

#include "makestep.h"

#include "qt4project.h"
#include "qt4target.h"
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/gnumakeparser.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

using ProjectExplorer::Environment;
using ExtensionSystem::PluginManager;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
const char * const MAKESTEP_BS_ID("Qt4ProjectManager.MakeStep");

const char * const MAKE_ARGUMENTS_KEY("Qt4ProjectManager.MakeStep.MakeArguments");
const char * const MAKE_COMMAND_KEY("Qt4ProjectManager.MakeStep.MakeCommand");
const char * const CLEAN_KEY("Qt4ProjectManager.MakeStep.Clean");
}

MakeStep::MakeStep(ProjectExplorer::BuildConfiguration *bc) :
    AbstractProcessStep(bc, QLatin1String(MAKESTEP_BS_ID)),
    m_clean(false)
{
    ctor();
}

MakeStep::MakeStep(ProjectExplorer::BuildConfiguration *bc, MakeStep *bs) :
    AbstractProcessStep(bc, bs),
    m_clean(bs->m_clean),
    m_userArgs(bs->m_userArgs),
    m_makeCmd(bs->m_makeCmd)
{
    ctor();
}

MakeStep::MakeStep(ProjectExplorer::BuildConfiguration *bc, const QString &id) :
    AbstractProcessStep(bc, id),
    m_clean(false)
{
    ctor();
}

void MakeStep::ctor()
{
    setDisplayName(tr("Make", "Qt4 MakeStep display name."));
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
    m_userArgs = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toStringList();
    m_clean = map.value(QLatin1String(CLEAN_KEY)).toBool();

    return BuildStep::fromMap(map);
}

bool MakeStep::init()
{
    Qt4BuildConfiguration *bc = qt4BuildConfiguration();
    Environment environment = bc->environment();
    setEnvironment(environment);

    QString workingDirectory;
    if (bc->subNodeBuild())
        workingDirectory = bc->subNodeBuild()->buildDir();
    else
        workingDirectory = bc->buildDirectory();
    setWorkingDirectory(workingDirectory);

    QString makeCmd = bc->makeCommand();
    if (!m_makeCmd.isEmpty())
        makeCmd = m_makeCmd;
    if (!QFileInfo(makeCmd).isAbsolute()) {
        // Try to detect command in environment
        const QString tmp = environment.searchInPath(makeCmd);
        if (tmp.isEmpty()) {
            emit addOutput(tr("<font color=\"#ff0000\">Could not find make command: %1 "\
                              "in the build environment</font>").arg(makeCmd));
            return false;
        }
        makeCmd = tmp;
    }
    setCommand(makeCmd);

    // If we are cleaning, then make can fail with a error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on a already clean project
    setIgnoreReturnValue(m_clean);
    QStringList args = m_userArgs;
    if (!m_clean) {
        if (!bc->defaultMakeTarget().isEmpty())
            args << bc->defaultMakeTarget();
    }
    // -w option enables "Enter"/"Leaving directory" messages, which we need for detecting the
    // absolute file path
    // FIXME doing this without the user having a way to override this is rather bad
    // so we only do it for unix and if the user didn't override the make command
    // but for now this is the least invasive change
    ProjectExplorer::ToolChain *toolchain = bc->toolChain();

    if (toolchain) {
        if (toolchain->type() != ProjectExplorer::ToolChain::MSVC &&
            toolchain->type() != ProjectExplorer::ToolChain::WINCE) {
            if (m_makeCmd.isEmpty())
                args << "-w";
        }
    }

    setEnabled(true);
    setArguments(args);

    setOutputParser(new ProjectExplorer::GnuMakeParser(workingDirectory));
    if (toolchain)
        appendOutputParser(toolchain->outputParser());

    return AbstractProcessStep::init();
}

void MakeStep::run(QFutureInterface<bool> & fi)
{
    if (qt4BuildConfiguration()->qt4Target()->qt4Project()->rootProjectNode()->projectType() == ScriptTemplate) {
        fi.reportResult(true);
        return;
    }

    AbstractProcessStep::run(fi);
}

bool MakeStep::immutable() const
{
    return false;
}

ProjectExplorer::BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeStepConfigWidget(this);
}

QStringList MakeStep::userArguments()
{
    return m_userArgs;
}

void MakeStep::setUserArguments(const QStringList &arguments)
{
    m_userArgs = arguments;
    emit userArgumentsChanged();
}

MakeStepConfigWidget::MakeStepConfigWidget(MakeStep *makeStep)
    : BuildStepConfigWidget(), m_makeStep(makeStep), m_ignoreChange(false)
{
    m_ui.setupUi(this);
    connect(m_ui.makeLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeEdited()));
    connect(m_ui.makeArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeArgumentsLineEdited()));

    connect(makeStep, SIGNAL(userArgumentsChanged()),
            this, SLOT(userArgumentsChanged()));
    connect(makeStep->buildConfiguration(), SIGNAL(buildDirectoryChanged()),
            this, SLOT(updateDetails()));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrideLabel()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
}

void MakeStepConfigWidget::updateMakeOverrideLabel()
{
    Qt4BuildConfiguration *qt4bc = m_makeStep->qt4BuildConfiguration();
    m_ui.makeLabel->setText(tr("Override %1:").arg(qt4bc->makeCommand()));
}

void MakeStepConfigWidget::updateDetails()
{
    Qt4BuildConfiguration *bc = m_makeStep->qt4BuildConfiguration();
    QString workingDirectory = bc->buildDirectory();

    QString makeCmd = bc->makeCommand();
    if (!m_makeStep->m_makeCmd.isEmpty())
        makeCmd = m_makeStep->m_makeCmd;
    if (!QFileInfo(makeCmd).isAbsolute()) {
        Environment environment = bc->environment();
        // Try to detect command in environment
        const QString tmp = environment.searchInPath(makeCmd);
        if (tmp.isEmpty()) {
            m_summaryText = tr("<b>Make Step:</b> %1 not found in the environment.").arg(makeCmd);
            emit updateSummary();
            return;
        }
        makeCmd = tmp;
    }
    // -w option enables "Enter"/"Leaving directory" messages, which we need for detecting the
    // absolute file path
    // FIXME doing this without the user having a way to override this is rather bad
    // so we only do it for unix and if the user didn't override the make command
    // but for now this is the least invasive change
    QStringList args = m_makeStep->userArguments();
    ProjectExplorer::ToolChain::ToolChainType t = ProjectExplorer::ToolChain::UNKNOWN;
    ProjectExplorer::ToolChain *toolChain = bc->toolChain();
    if (toolChain)
        t = toolChain->type();
    if (t != ProjectExplorer::ToolChain::MSVC && t != ProjectExplorer::ToolChain::WINCE) {
        if (m_makeStep->m_makeCmd.isEmpty())
            args << "-w";
    }
    m_summaryText = tr("<b>Make:</b> %1 %2 in %3").arg(QFileInfo(makeCmd).fileName(), args.join(" "),
                                                       QDir::toNativeSeparators(workingDirectory));
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
    const QStringList &makeArguments = m_makeStep->userArguments();
    m_ui.makeArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(makeArguments));
    updateDetails();
}

void MakeStepConfigWidget::init()
{
    updateMakeOverrideLabel();

    const QString &makeCmd = m_makeStep->m_makeCmd;
    m_ui.makeLineEdit->setText(makeCmd);

    const QStringList &makeArguments = m_makeStep->userArguments();
    m_ui.makeArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(makeArguments));
    updateDetails();
}

void MakeStepConfigWidget::makeEdited()
{
    m_makeStep->m_makeCmd = m_ui.makeLineEdit->text();
    updateDetails();
}

void MakeStepConfigWidget::makeArgumentsLineEdited()
{
    m_ignoreChange = true;
    m_makeStep->setUserArguments(
            ProjectExplorer::Environment::parseCombinedArgString(m_ui.makeArgumentsLineEdit->text()));
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

bool MakeStepFactory::canCreate(ProjectExplorer::BuildConfiguration *parent, const QString &id) const
{
    if (!qobject_cast<Qt4BuildConfiguration *>(parent))
        return false;
    return (id == QLatin1String(MAKESTEP_BS_ID));
}

ProjectExplorer::BuildStep *MakeStepFactory::create(ProjectExplorer::BuildConfiguration *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new MakeStep(parent);
}

bool MakeStepFactory::canClone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::BuildStep *MakeStepFactory::clone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new MakeStep(parent, static_cast<MakeStep *>(source));
}

bool MakeStepFactory::canRestore(ProjectExplorer::BuildConfiguration *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

ProjectExplorer::BuildStep *MakeStepFactory::restore(ProjectExplorer::BuildConfiguration *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    MakeStep *bs(new MakeStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList MakeStepFactory::availableCreationIds(ProjectExplorer::BuildConfiguration *parent) const
{
    if (qobject_cast<Qt4BuildConfiguration *>(parent))
        return QStringList() << QLatin1String(MAKESTEP_BS_ID);
    return QStringList();
}

QString MakeStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(MAKESTEP_BS_ID))
        return tr("Make");
    return QString();
}
