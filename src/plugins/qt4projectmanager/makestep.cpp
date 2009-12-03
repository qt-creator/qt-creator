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
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

using ProjectExplorer::IBuildParserFactory;
using ProjectExplorer::IBuildParser;
using ProjectExplorer::Environment;
using ExtensionSystem::PluginManager;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

MakeStep::MakeStep(ProjectExplorer::BuildConfiguration *bc)
    : AbstractMakeStep(bc), m_clean(false)
{

}

MakeStep::MakeStep(MakeStep *bs, ProjectExplorer::BuildConfiguration *bc)
    : AbstractMakeStep(bs, bc),
    m_clean(bs->m_clean),
    m_userArgs(bs->m_userArgs),
    m_makeCmd(bs->m_makeCmd)
{

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

void MakeStep::restoreFromGlobalMap(const QMap<QString, QVariant> &map)
{
    if (map.value("clean").isValid() && map.value("clean").toBool())
        m_clean = true;
    ProjectExplorer::AbstractMakeStep::restoreFromGlobalMap(map);
}

void MakeStep::restoreFromLocalMap(const QMap<QString, QVariant> &map)
{
    m_userArgs = map.value("makeargs").toStringList();
    m_makeCmd = map.value("makeCmd").toString();
    if (map.value("clean").isValid() && map.value("clean").toBool())
        m_clean = true;
    ProjectExplorer::AbstractMakeStep::restoreFromLocalMap(map);
}

void MakeStep::storeIntoLocalMap(QMap<QString, QVariant> &map)
{
    map["makeargs"] = m_userArgs;
    map["makeCmd"] = m_makeCmd;
    if (m_clean)
        map["clean"] = true;
    ProjectExplorer::AbstractMakeStep::storeIntoLocalMap(map);
}

bool MakeStep::init()
{
    Qt4BuildConfiguration *bc = qt4BuildConfiguration();
    Environment environment = bc->environment();
    setEnvironment(environment);

    QString workingDirectory = bc->buildDirectory();
    setWorkingDirectory(workingDirectory);

    QString makeCmd = bc->makeCommand();
    if (!m_makeCmd.isEmpty())
        makeCmd = m_makeCmd;
    if (!QFileInfo(makeCmd).isAbsolute()) {
        // Try to detect command in environment
        QString tmp = environment.searchInPath(makeCmd);
        if (tmp == QString::null) {
            emit addToOutputWindow(tr("<font color=\"#ff0000\">Could not find make command: %1 "\
                                      "in the build environment</font>").arg(makeCmd));
            return false;
        }
        makeCmd = tmp;
    }
    setCommand(makeCmd);

    // If we are cleaning, then make can fail with a error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on a alrady clean project
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

    ProjectExplorer::ToolChain::ToolChainType type =  ProjectExplorer::ToolChain::UNKNOWN;
    if (toolchain)
        type = toolchain->type();
    if (type != ProjectExplorer::ToolChain::MSVC && type != ProjectExplorer::ToolChain::WINCE) {
        if (m_makeCmd.isEmpty())
            args << "-w";
    }

    setEnabled(true);
    setArguments(args);

    if (type == ProjectExplorer::ToolChain::MSVC || type == ProjectExplorer::ToolChain::WINCE)
        setBuildParser(ProjectExplorer::Constants::BUILD_PARSER_MSVC);
    else if (ProjectExplorer::ToolChain::GCCE == type)
        setBuildParser(ProjectExplorer::Constants::BUILD_PARSER_ABLD_GCCE);
    else if (ProjectExplorer::ToolChain::WINSCW == type)
        setBuildParser(ProjectExplorer::Constants::BUILD_PARSER_ABLD_WINSCW);
    else if (ProjectExplorer::ToolChain::RVCT_ARMV5 == type ||
             ProjectExplorer::ToolChain::RVCT_ARMV6 == type)
        setBuildParser(ProjectExplorer::Constants::BUILD_PARSER_ABLD_RVCT);
    else
        setBuildParser(ProjectExplorer::Constants::BUILD_PARSER_GCC);

    return AbstractMakeStep::init();
}

void MakeStep::run(QFutureInterface<bool> & fi)
{
    if (qt4BuildConfiguration()->qt4Project()->rootProjectNode()->projectType() == ScriptTemplate) {
        fi.reportResult(true);
        return;
    }

    AbstractMakeStep::run(fi);
}

QString MakeStep::name()
{
    return Constants::MAKESTEP;
}

QString MakeStep::displayName()
{
    return "Make";
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
        QString tmp = environment.searchInPath(makeCmd);
        if (tmp == QString::null) {
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
// MakeStep
///

MakeStepFactory::MakeStepFactory()
{
}

MakeStepFactory::~MakeStepFactory()
{
}

bool MakeStepFactory::canCreate(const QString & name) const
{
    return (name == Constants::MAKESTEP);
}

ProjectExplorer::BuildStep *MakeStepFactory::create(ProjectExplorer::BuildConfiguration *bc, const QString & name) const
{
    Q_UNUSED(name)
    return new MakeStep(bc);
}

ProjectExplorer::BuildStep *MakeStepFactory::clone(ProjectExplorer::BuildStep *bs, ProjectExplorer::BuildConfiguration *bc) const
{
    return new MakeStep(static_cast<MakeStep *>(bs), bc);
}

QStringList MakeStepFactory::canCreateForBuildConfiguration(ProjectExplorer::BuildConfiguration *pro) const
{
    if (qobject_cast<Qt4BuildConfiguration *>(pro))
        return QStringList() << Constants::MAKESTEP;
    else
        return QStringList();
}

QString MakeStepFactory::displayNameForName(const QString &name) const
{
    Q_UNUSED(name)
    return tr("Make");
}
