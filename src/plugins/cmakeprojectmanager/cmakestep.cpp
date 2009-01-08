/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "cmakestep.h"

#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <utils/qtcassert.h>

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
    setCommand(buildConfiguration, "cmake"); // TODO give full path here?
    setArguments(buildConfiguration, QStringList() << "-GUnix Makefiles"); // TODO
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
    return "CMake";
}

QString CMakeStep::displayName()
{
    return Constants::CMAKESTEP;
}

ProjectExplorer::BuildStepConfigWidget *CMakeStep::createConfigWidget()
{
    return new CMakeBuildStepConfigWidget();
}

bool CMakeStep::immutable() const
{
    return true;
}

//
// CMakeBuildStepConfigWidget
//

QString CMakeBuildStepConfigWidget::displayName() const
{
    return "CMake";
}

void CMakeBuildStepConfigWidget::init(const QString & /*buildConfiguration */)
{
    // TODO
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

