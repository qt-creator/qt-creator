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
using ProjectExplorer::BuildParserInterface;
using ProjectExplorer::Environment;
using ExtensionSystem::PluginManager;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

MakeStep::MakeStep(Qt4Project * project)
    : AbstractMakeStep(project)
{

}

MakeStep::~MakeStep()
{

}

bool MakeStep::init(const QString &name)
{
    m_buildConfiguration = name;
    ProjectExplorer::BuildConfiguration *bc = project()->buildConfiguration(name);
    Environment environment = project()->environment(bc);
    setEnvironment(name, environment);

    Qt4Project *qt4project = qobject_cast<Qt4Project *>(project());
    QString workingDirectory = qt4project->buildDirectory(bc);
    setWorkingDirectory(name, workingDirectory);

    QString makeCmd = qt4project->makeCommand(bc);
    if (!value(name, "makeCmd").toString().isEmpty())
        makeCmd = value(name, "makeCmd").toString();
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
    setCommand(name, makeCmd);

    if (!value(name, "cleanConfig").isValid() && value("clean").isValid() && value("clean").toBool()) {
        // Import old settings
        setValue(name, "cleanConfig", true);
        setValue(name, "makeargs", QStringList() << "clean");
    }
    // If we are cleaning, then make can fail with a error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on a alrady clean project
    setIgnoreReturnValue(name, value(name, "cleanConfig").isValid());
    QStringList args = value(name, "makeargs").toStringList();
    if (!value(name, "cleanConfig").isValid()) {
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
        if (value(name, "makeCmd").toString().isEmpty())
            args << "-w";
    }

    setEnabled(name, true);
    setArguments(name, args);

    if (type == ProjectExplorer::ToolChain::MSVC || type == ProjectExplorer::ToolChain::WINCE)
        setBuildParser(ProjectExplorer::Constants::BUILD_PARSER_MSVC);
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

    if (!enabled(m_buildConfiguration)) {
        emit addToOutputWindow(tr("<font color=\"#0000ff\"><b>No Makefile found, assuming project is clean.</b></font>"));
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

void MakeStep::setMakeArguments(const QString &buildConfiguration, const QStringList &arguments)
{
    setValue(buildConfiguration, "makeargs", arguments);
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
    // TODO reduce heavy code duplication
    Qt4Project *pro = static_cast<Qt4Project *>(m_makeStep->project());
    ProjectExplorer::BuildConfiguration *bc = pro->buildConfiguration(m_buildConfiguration);
    QString workingDirectory = pro->buildDirectory(bc);

    QString makeCmd = pro->makeCommand(bc);
    if (!m_makeStep->value(m_buildConfiguration, "makeCmd").toString().isEmpty())
        makeCmd = m_makeStep->value(m_buildConfiguration, "makeCmd").toString();
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
    QStringList args = m_makeStep->value(m_buildConfiguration, "makeargs").toStringList();
    ProjectExplorer::ToolChain::ToolChainType t = ProjectExplorer::ToolChain::UNKNOWN;
    ProjectExplorer::ToolChain *toolChain = pro->toolChain(bc);
    if (toolChain)
        t = toolChain->type();
    if (t != ProjectExplorer::ToolChain::MSVC && t != ProjectExplorer::ToolChain::WINCE) {
        if (m_makeStep->value(m_buildConfiguration, "makeCmd").toString().isEmpty())
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

    Qt4Project *pro = qobject_cast<Qt4Project *>(m_makeStep->project());
    Q_ASSERT(pro);

    if (!m_makeStep->value(buildConfiguration, "cleanConfig").isValid() && m_makeStep->value("clean").isValid() && m_makeStep->value("clean").toBool()) {
        // Import old settings
        m_makeStep->setValue(buildConfiguration, "cleanConfig", true);
        m_makeStep->setValue(buildConfiguration, "makeargs", QStringList() << "clean");
    }

    updateMakeOverrideLabel();

    const QString &makeCmd = m_makeStep->value(buildConfiguration, "makeCmd").toString();
    m_ui.makeLineEdit->setText(makeCmd);

    const QStringList &makeArguments =
            m_makeStep->value(buildConfiguration, "makeargs").toStringList();
    m_ui.makeArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(makeArguments));
    updateDetails();
}

void MakeStepConfigWidget::makeLineEditTextEdited()
{
    Q_ASSERT(!m_buildConfiguration.isNull());
    m_makeStep->setValue(m_buildConfiguration, "makeCmd", m_ui.makeLineEdit->text());
    updateDetails();
}

void MakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    Q_ASSERT(!m_buildConfiguration.isNull());
    m_makeStep->setValue(m_buildConfiguration, "makeargs",
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
