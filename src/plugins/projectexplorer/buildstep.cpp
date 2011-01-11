/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "buildstep.h"

#include "buildconfiguration.h"
#include "buildsteplist.h"
#include "target.h"

using namespace ProjectExplorer;

BuildStep::BuildStep(BuildStepList *bsl, const QString &id) :
    ProjectConfiguration(bsl, id)
{
    Q_ASSERT(bsl);
}

BuildStep::BuildStep(BuildStepList *bsl, BuildStep *bs) :
    ProjectConfiguration(bsl, bs)
{
    Q_ASSERT(bsl);
}

BuildStep::~BuildStep()
{
}

BuildConfiguration *BuildStep::buildConfiguration() const
{
    BuildConfiguration *bc = qobject_cast<BuildConfiguration *>(parent()->parent());
    if (!bc)
        bc = target()->activeBuildConfiguration();
    return bc;
}

Target *BuildStep::target() const
{
    return qobject_cast<Target *>(parent()->parent()->parent());
}

bool BuildStep::immutable() const
{
    return false;
}

IBuildStepFactory::IBuildStepFactory(QObject *parent) :
    QObject(parent)
{ }

IBuildStepFactory::~IBuildStepFactory()
{ }
