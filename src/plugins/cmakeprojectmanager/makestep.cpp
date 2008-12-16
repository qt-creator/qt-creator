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

#include "makestep.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"

#include <utils/qtcassert.h>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

MakeStep::MakeStep(CMakeProject *pro)
    : AbstractProcessStep(pro), m_pro(pro)
{
}

MakeStep::~MakeStep()
{
}

bool MakeStep::init(const QString &buildConfiguration)
{
    setEnabled(buildConfiguration, true);
    setWorkingDirectory(buildConfiguration, m_pro->buildDirectory(buildConfiguration));
    setCommand(buildConfiguration, "make"); // TODO give full path here?
    setArguments(buildConfiguration, QStringList()); // TODO
    setEnvironment(buildConfiguration, m_pro->environment(buildConfiguration));
    return AbstractProcessStep::init(buildConfiguration);
}

void MakeStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

QString MakeStep::name()
{
    return "Make";
}

QString MakeStep::displayName()
{
    return Constants::CMAKESTEP;
}

ProjectExplorer::BuildStepConfigWidget *MakeStep::createConfigWidget()
{
    return new MakeBuildStepConfigWidget();
}

bool MakeStep::immutable() const
{
    return true;
}

//
// CMakeBuildStepConfigWidget
//

QString MakeBuildStepConfigWidget::displayName() const
{
    return "Make";
}

void MakeBuildStepConfigWidget::init(const QString &buildConfiguration)
{
    // TODO
}

//
// MakeBuildStepFactory
//

bool MakeBuildStepFactory::canCreate(const QString &name) const
{
    return (Constants::MAKESTEP == name);
}

ProjectExplorer::BuildStep *MakeBuildStepFactory::create(ProjectExplorer::Project *project, const QString &name) const
{
    QTC_ASSERT(name == Constants::MAKESTEP, return 0);
    CMakeProject *pro = qobject_cast<CMakeProject *>(project);
    QTC_ASSERT(pro, return 0);
    return new MakeStep(pro);
}

QStringList MakeBuildStepFactory::canCreateForProject(ProjectExplorer::Project *pro) const
{
    return QStringList();
}

QString MakeBuildStepFactory::displayNameForName(const QString &name) const
{
    return "Make";
}

