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

#include "buildstep.h"
#include "buildconfiguration.h"
#include "project.h"

#include <utils/qtcassert.h>
#include <QtGui/QLayout>

using namespace ProjectExplorer;

BuildStep::BuildStep(Project * pro)
    : m_project(pro)
{
}

BuildStep::~BuildStep()
{

}


void BuildStep::restoreFromMap(const QMap<QString, QVariant> &map)
{
    Q_UNUSED(map)
}

void BuildStep::storeIntoMap(QMap<QString, QVariant> &map)
{
    Q_UNUSED(map)
}

void BuildStep::restoreFromMap(const QString &buildConfiguration, const QMap<QString, QVariant> &map)
{
    Q_UNUSED(buildConfiguration)
    Q_UNUSED(map)
}

void BuildStep::storeIntoMap(const QString &buildConfiguration, QMap<QString, QVariant> &map)
{
    Q_UNUSED(buildConfiguration)
    Q_UNUSED(map)
}

Project * BuildStep::project() const
{
    return m_project;
}

void BuildStep::addBuildConfiguration(const QString &name)
{
    Q_UNUSED(name)
}

void BuildStep::removeBuildConfiguration(const QString &name)
{
    Q_UNUSED(name)
}

void BuildStep::copyBuildConfiguration(const QString &source, const QString &dest)
{
    Q_UNUSED(source)
    Q_UNUSED(dest)
}

bool BuildStep::immutable() const
{
    return false;
}

IBuildStepFactory::IBuildStepFactory()
{

}

IBuildStepFactory::~IBuildStepFactory()
{

}
