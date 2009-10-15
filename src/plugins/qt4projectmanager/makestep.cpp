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

MakeStep::MakeStep(Qt4Project * project)
    : AbstractMakeStep(project), m_clean(false)
{

}

MakeStep::~MakeStep()
{

}

void MakeStep::setClean(bool clean)
{
    m_clean = clean;
}

void MakeStep::restoreFromMap(const QMap<QString, QVariant> &map)
{
    if (map.value("clean").isValid() && map.value("clean").toBool())
        m_clean = true;
    ProjectExplorer::AbstractMakeStep::restoreFromMap(map);
}

void MakeStep::storeIntoMap(QMap<QString, QVariant> &map)
{
    if (m_clean)
        map["clean"] = true;
    ProjectExplorer::AbstractMakeStep::storeIntoMap(map);
}

void MakeStep::restoreFromMap(const QString &buildConfiguration, const QMap<QString, QVariant> &map)
{
    m_values[buildConfiguration].makeargs = map.value("makeargs").toStringList();
    m_values[buildConfiguration].makeCmd = map.value("makeCmd").toString();
    ProjectExplorer::AbstractMakeStep::restoreFromMap(buildConfiguration, map);
}

void MakeStep::storeIntoMap(const QString &buildConfiguration, QMap<QString, QVariant> &map)
{
    map["makeargs"] = m_values.value(buildConfiguration).makeargs;
    map["makeCmd"] = m_values.value(buildConfiguration).makeCmd;
    ProjectExplorer::AbstractMakeStep::storeIntoMap(buildConfiguration, map);
}

void MakeStep::addBuildConfiguration(const QString & name)
{
    m_values.insert(name, MakeStepSettings());
}

void MakeStep::removeBuildConfiguration(const QString & name)
{
    m_values.remove(name);
}

void MakeStep::copyBuildConfiguration(const QString &source, const QString &dest)
{
    m_values.insert(dest, m_values.value(source));
}


bool MakeStep::init(const QString &name)
{
    m_buildConfiguration = name;
    ProjectExplorer::BuildConfiguration *bc = project()->buildConfiguration(name);
    Environment environment = project()->environment(bc);
    setEnvironment(environment);

    Qt4Project *qt4project = qobject_cast<Qt4Project *>(project());
    QString workingDirectory = qt4project->buildDirectory(bc);
    setWorkingDirectory(workingDirectory);

    QString makeCmd = qt4project->makeCommand(bc);
    if (!m_values.value(name).makeCmd.isEmpty())
        makeCmd = m_values.value(name).makeCmd;
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
    QStringList args = m_values.value(name).makeargs;
    if (!m_clean) {
        if (!qt4project->defaultMakeTarget(bc).isEmpty())
            args << qt4project->defaultMakeTarget(bc);
    }
    // -w option enables "Enter"/"Leaving directory" messages, which we need for detecting the
    // absolute file path
    // FIXME doing this without the user having a way to override this is rather bad
    // so we only do it for unix and if the user didn't override the make command
    // but for now this is the least invasive change
    ProjectExplorer::ToolChain *toolchain = qt4project->toolChain(bc);

    ProjectExplorer::ToolChain::ToolChainType type =  ProjectExplorer::ToolChain::UNKNOWN;
    if (toolchain)
        type = toolchain->type();
    if (type != ProjectExplorer::ToolChain::MSVC && type != ProjectExplorer::ToolChain::WINCE) {
        if (m_values.value(name).makeCmd.isEmpty())
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

    return AbstractMakeStep::init(name);
}

void MakeStep::run(QFutureInterface<bool> & fi)
{
    if (qobject_cast<Qt4Project *>(project())->rootProjectNode()->projectType() == ScriptTemplate) {
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

QStringList MakeStep::makeArguments(const QString &buildConfiguration)
{
    return m_values.value(buildConfiguration).makeargs;
}

void MakeStep::setMakeArguments(const QString &buildConfiguration, const QStringList &arguments)
{
    m_values[buildConfiguration].makeargs = arguments;
    emit changed();
}

MakeStepConfigWidget::MakeStepConfigWidget(MakeStep *makeStep)
    : BuildStepConfigWidget(), m_makeStep(makeStep)
{
    m_ui.setupUi(this);
    connect(m_ui.makeLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeLineEditTextEdited()));
    connect(m_ui.makeArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeArgumentsLineEditTextEdited()));

    connect(makeStep, SIGNAL(changed()),
            this, SLOT(update()));
    connect(makeStep->project(), SIGNAL(buildDirectoryChanged()),
            this, SLOT(updateDetails()));

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateMakeOverrideLabel()));
    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
}

void MakeStepConfigWidget::updateMakeOverrideLabel()
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(m_makeStep->project());
    m_ui.makeLabel->setText(tr("Override %1:").arg(qt4project->
        makeCommand(qt4project->buildConfiguration(m_buildConfiguration))));
}

void MakeStepConfigWidget::updateDetails()
{
    Qt4Project *pro = static_cast<Qt4Project *>(m_makeStep->project());
    ProjectExplorer::BuildConfiguration *bc = pro->buildConfiguration(m_buildConfiguration);
    QString workingDirectory = pro->buildDirectory(bc);

    QString makeCmd = pro->makeCommand(bc);
    if (!m_makeStep->m_values.value(m_buildConfiguration).makeCmd.isEmpty())
        makeCmd = m_makeStep->m_values.value(m_buildConfiguration).makeCmd;
    if (!QFileInfo(makeCmd).isAbsolute()) {
        Environment environment = pro->environment(bc);
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
    QStringList args = m_makeStep->makeArguments(m_buildConfiguration);
    ProjectExplorer::ToolChain::ToolChainType t = ProjectExplorer::ToolChain::UNKNOWN;
    ProjectExplorer::ToolChain *toolChain = pro->toolChain(bc);
    if (toolChain)
        t = toolChain->type();
    if (t != ProjectExplorer::ToolChain::MSVC && t != ProjectExplorer::ToolChain::WINCE) {
        if (m_makeStep->m_values.value(m_buildConfiguration).makeCmd.isEmpty())
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

void MakeStepConfigWidget::update()
{
    init(m_buildConfiguration);
}

void MakeStepConfigWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;

    updateMakeOverrideLabel();

    const QString &makeCmd = m_makeStep->m_values.value(buildConfiguration).makeCmd;
    m_ui.makeLineEdit->setText(makeCmd);

    const QStringList &makeArguments = m_makeStep->makeArguments(buildConfiguration);
    m_ui.makeArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(makeArguments));
    updateDetails();
}

void MakeStepConfigWidget::makeLineEditTextEdited()
{
    Q_ASSERT(!m_buildConfiguration.isNull());
    m_makeStep->m_values[m_buildConfiguration].makeCmd = m_ui.makeLineEdit->text();
    updateDetails();
}

void MakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    Q_ASSERT(!m_buildConfiguration.isNull());
    m_makeStep->setMakeArguments(m_buildConfiguration,
                         ProjectExplorer::Environment::parseCombinedArgString(m_ui.makeArgumentsLineEdit->text()));
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

ProjectExplorer::BuildStep * MakeStepFactory::create(ProjectExplorer::Project * pro, const QString & name) const
{
    Q_UNUSED(name)
    return new MakeStep(static_cast<Qt4Project *>(pro));
}

QStringList MakeStepFactory::canCreateForProject(ProjectExplorer::Project *pro) const
{
    if (qobject_cast<Qt4Project *>(pro))
        return QStringList() << Constants::MAKESTEP;
    else
        return QStringList();
}

QString MakeStepFactory::displayNameForName(const QString &name) const
{
    Q_UNUSED(name)
    return tr("Make");
}
