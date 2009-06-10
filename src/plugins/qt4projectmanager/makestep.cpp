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
** contact the sales department at http://www.qtsoftware.com/contact.
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
    Environment environment = project()->environment(name);
    setEnvironment(name, environment);

    QString workingDirectory;
    if (project()->value(name, "useShadowBuild").toBool())
        workingDirectory = project()->value(name, "buildDirectory").toString();
    if (workingDirectory.isEmpty())
        workingDirectory = QFileInfo(project()->file()->fileName()).absolutePath();
    setWorkingDirectory(name, workingDirectory);

    Qt4Project *qt4project = qobject_cast<Qt4Project *>(project());
    QString makeCmd = qt4project->makeCommand(name);
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

    bool skipMakeClean = false;
    QStringList args;
    if (value("clean").isValid() && value("clean").toBool())  {
        args = QStringList() << "clean";
        if (!QDir(workingDirectory).exists(QLatin1String("Makefile"))) {
            skipMakeClean = true;
        }
    } else {
        args = value(name, "makeargs").toStringList();
        if (!qt4project->defaultMakeTarget(name).isEmpty())
            args << qt4project->defaultMakeTarget(name);
    }

    // -w option enables "Enter"/"Leaving directory" messages, which we need for detecting the
    // absolute file path
    // FIXME doing this without the user having a way to override this is rather bad
    // so we only do it for unix and if the user didn't override the make command
    // but for now this is the least invasive change
    ProjectExplorer::ToolChain::ToolChainType t = qobject_cast<Qt4Project *>(project())->toolChain(name)->type();
    if (t != ProjectExplorer::ToolChain::MSVC && t != ProjectExplorer::ToolChain::WINCE) {
        if (value(name, "makeCmd").toString().isEmpty())
            args << "-w";
    }

    setEnabled(name, !skipMakeClean);
    setArguments(name, args);

    ProjectExplorer::ToolChain::ToolChainType type = qobject_cast<Qt4Project *>(project())->toolChain(name)->type();
    if ( type == ProjectExplorer::ToolChain::MSVC || type == ProjectExplorer::ToolChain::WINCE)
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
    return true;
}

ProjectExplorer::BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeStepConfigWidget(this);
}

MakeStepConfigWidget::MakeStepConfigWidget(MakeStep *makeStep)
    : BuildStepConfigWidget(), m_makeStep(makeStep)
{
    m_ui.setupUi(this);
    connect(m_ui.makeLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeLineEditTextEdited()));
    connect(m_ui.makeArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(makeArgumentsLineEditTextEdited()));
}

QString MakeStepConfigWidget::displayName() const
{
    return m_makeStep->displayName();
}

void MakeStepConfigWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;
    bool showPage0 = buildConfiguration.isNull();
    m_ui.stackedWidget->setCurrentIndex(showPage0 ? 0 : 1);

    if (!showPage0) {
        Qt4Project *pro = qobject_cast<Qt4Project *>(m_makeStep->project());
        Q_ASSERT(pro);
        m_ui.makeLabel->setText(tr("Override %1:").arg(pro->makeCommand(buildConfiguration)));

        const QString &makeCmd = m_makeStep->value(buildConfiguration, "makeCmd").toString();
        m_ui.makeLineEdit->setText(makeCmd);

        const QStringList &makeArguments =
            m_makeStep->value(buildConfiguration, "makeargs").toStringList();
        m_ui.makeArgumentsLineEdit->setText(ProjectExplorer::Environment::joinArgumentList(makeArguments));
    }
}

void MakeStepConfigWidget::makeLineEditTextEdited()
{
    Q_ASSERT(!m_buildConfiguration.isNull());
    m_makeStep->setValue(m_buildConfiguration, "makeCmd", m_ui.makeLineEdit->text());
}

void MakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    Q_ASSERT(!m_buildConfiguration.isNull());
    m_makeStep->setValue(m_buildConfiguration, "makeargs",
                         ProjectExplorer::Environment::parseCombinedArgString(m_ui.makeArgumentsLineEdit->text()));
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
    Q_UNUSED(name);
    return new MakeStep(static_cast<Qt4Project *>(pro));
}

QStringList MakeStepFactory::canCreateForProject(ProjectExplorer::Project *pro) const
{
    Q_UNUSED(pro)
    return QStringList();
}

QString MakeStepFactory::displayNameForName(const QString &name) const
{
    Q_UNUSED(name);
    return QString();
}
