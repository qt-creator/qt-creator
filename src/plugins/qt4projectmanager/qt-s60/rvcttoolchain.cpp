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

#include "rvcttoolchain.h"

#include "qt4project.h"

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

RVCTToolChain::RVCTToolChain(S60Devices::Device device, ToolChain::ToolChainType type,
                             const QString &makeTargetBase)
    : m_deviceId(device.id),
    m_deviceName(device.name),
    m_deviceRoot(device.epocRoot),
    m_type(type),
    m_makeTargetBase(makeTargetBase)
{
}

ToolChain::ToolChainType RVCTToolChain::type() const
{
    return m_type;
}

QByteArray RVCTToolChain::predefinedMacros()
{
    //TODO
    return QByteArray();
}

QList<HeaderPath> RVCTToolChain::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        //TODO system header paths (from environment variables?)
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\stdapis").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\stdapis\\sys").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\variant").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
    }
    return m_systemHeaderPaths;
}

void RVCTToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    env.prependOrSetPath(QString("%1\\epoc32\\tools").arg(m_deviceRoot)); // e.g. make.exe
    env.prependOrSetPath(QString("%1\\epoc32\\gcc\\bin").arg(m_deviceRoot)); // e.g. gcc.exe
    env.set("EPOCDEVICE", QString("%1:%2").arg(m_deviceId, m_deviceName));
    env.set("EPOCROOT", S60Devices::cleanedRootPath(m_deviceRoot));
}

QString RVCTToolChain::makeCommand() const
{
    return "make";
}

QString RVCTToolChain::defaultMakeTarget() const
{
    const Qt4Project *qt4project = qobject_cast<const Qt4Project *>(m_project);
    if (qt4project) {
        if (!(QtVersion::QmakeBuildConfig(qt4project->value(
                qt4project->activeBuildConfiguration(),
                "buildConfiguration").toInt()) & QtVersion::DebugBuild)) {
            return QString::fromLocal8Bit("release-%1").arg(m_makeTargetBase);
        }
    }
    return QString::fromLocal8Bit("debug-%1").arg(m_makeTargetBase);
}

bool RVCTToolChain::equals(ToolChain *other) const
{
    return (other->type() == type()
            && m_deviceId == static_cast<RVCTToolChain *>(other)->m_deviceId
            && m_deviceName == static_cast<RVCTToolChain *>(other)->m_deviceName);
}

void RVCTToolChain::setProject(const ProjectExplorer::Project *project)
{
    m_project = project;
}
