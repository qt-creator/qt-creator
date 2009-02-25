/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cmakestep.h"

#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <projectexplorer/environment.h>
#include <utils/qtcassert.h>
#include <QtGui/QFormLayout>
#include <QtGui/QLineEdit>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeStep::CMakeStep(CMakeProject *pro)
    : AbstractProcessStep(pro), m_pro(pro)
{
}

CMakeStep::~CMakeStep()
{
}

bool CMakeStep::init(const QString &buildConfiguration)
{
    setEnabled(buildConfiguration, true);
    setWorkingDirectory(buildConfiguration, m_pro->buildDirectory(buildConfiguration));

    CMakeManager *cmakeProjectManager = static_cast<CMakeManager *>(m_pro->projectManager());

    setCommand(buildConfiguration, cmakeProjectManager->cmakeExecutable());

    QString sourceDir = QFileInfo(m_pro->file()->fileName()).absolutePath();
    setArguments(buildConfiguration,
                 QStringList()
                 << sourceDir
                 << "-GUnix Makefiles"
                 << value(buildConfiguration, "userArguments").toStringList()); // TODO

    setEnvironment(buildConfiguration, m_pro->environment(buildConfiguration));
    return AbstractProcessStep::init(buildConfiguration);
}

void CMakeStep::run(QFutureInterface<bool> &fi)
{
    // TODO we want to only run cmake if the command line arguments or
    // the CmakeLists.txt has actually changed
    // And we want all of them to share the SAME command line arguments
    // Shadow building ruins this, hmm, hmm
    //
    AbstractProcessStep::run(fi);
}

QString CMakeStep::name()
{
    return Constants::CMAKESTEP;
}

QString CMakeStep::displayName()
{
    return "CMake";
}

ProjectExplorer::BuildStepConfigWidget *CMakeStep::createConfigWidget()
{
    return new CMakeBuildStepConfigWidget(this);
}

bool CMakeStep::immutable() const
{
    return true;
}

QStringList CMakeStep::userArguments(const QString &buildConfiguration) const
{
    return value(buildConfiguration, "userArguments").toStringList();
}

void CMakeStep::setUserArguments(const QString &buildConfiguration, const QStringList &arguments)
{
    setValue(buildConfiguration, "userArguments", arguments);
}

//
// CMakeBuildStepConfigWidget
//

CMakeBuildStepConfigWidget::CMakeBuildStepConfigWidget(CMakeStep *cmakeStep)
    : m_cmakeStep(cmakeStep)
{
    QFormLayout *fl = new QFormLayout(this);
    setLayout(fl);
    m_arguments = new QLineEdit(this);
    fl->addRow("Additional arguments", m_arguments);
    connect(m_arguments, SIGNAL(textChanged(QString)), this, SLOT(argumentsLineEditChanged()));
}

QString CMakeBuildStepConfigWidget::displayName() const
{
    return "CMake";
}

void CMakeBuildStepConfigWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;
    disconnect(m_arguments, SIGNAL(textChanged(QString)), this, SLOT(argumentsLineEditChanged()));
    m_arguments->setText(ProjectExplorer::Environment::joinArgumentList(m_cmakeStep->userArguments(buildConfiguration)));
    connect(m_arguments, SIGNAL(textChanged(QString)), this, SLOT(argumentsLineEditChanged()));
}

void CMakeBuildStepConfigWidget::argumentsLineEditChanged()
{
    m_cmakeStep->setUserArguments(m_buildConfiguration, ProjectExplorer::Environment::parseCombinedArgString(m_arguments->text()));
}

//
// CMakeBuildStepFactory
//

bool CMakeBuildStepFactory::canCreate(const QString &name) const
{
    return (Constants::CMAKESTEP == name);
}

ProjectExplorer::BuildStep *CMakeBuildStepFactory::create(ProjectExplorer::Project *project, const QString &name) const
{
    Q_ASSERT(name == Constants::CMAKESTEP);
    CMakeProject *pro = qobject_cast<CMakeProject *>(project);
    Q_ASSERT(pro);
    return new CMakeStep(pro);
}

QStringList CMakeBuildStepFactory::canCreateForProject(ProjectExplorer::Project * /* pro */) const
{
    return QStringList();
}

QString CMakeBuildStepFactory::displayNameForName(const QString & /* name */) const
{
    return "CMake";
}

