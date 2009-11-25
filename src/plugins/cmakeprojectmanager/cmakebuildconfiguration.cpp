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

#include "cmakebuildconfiguration.h"
#include "cmakeproject.h"
#include <projectexplorer/projectexplorerconstants.h>

using namespace CMakeProjectManager;
using namespace Internal;

CMakeBuildConfiguration::CMakeBuildConfiguration(CMakeProject *pro)
    : BuildConfiguration(pro), m_toolChain(0)
{

}

CMakeBuildConfiguration::CMakeBuildConfiguration(BuildConfiguration *source)
    : BuildConfiguration(source), m_toolChain(0)
{

}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{
    delete m_toolChain;
}

ProjectExplorer::Environment CMakeBuildConfiguration::baseEnvironment() const
{
    ProjectExplorer::Environment env = useSystemEnvironment() ?
                                       ProjectExplorer::Environment(QProcess::systemEnvironment()) :
                                       ProjectExplorer::Environment();
    return env;
}

ProjectExplorer::Environment CMakeBuildConfiguration::environment() const
{
    ProjectExplorer::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

void CMakeBuildConfiguration::setUseSystemEnvironment(bool b)
{
    if (b == useSystemEnvironment())
        return;
    setValue("clearSystemEnvironment", !b);
    emit environmentChanged();
}

bool CMakeBuildConfiguration::useSystemEnvironment() const
{
    bool b = !(value("clearSystemEnvironment").isValid() &&
               value("clearSystemEnvironment").toBool());
    return b;
}

QList<ProjectExplorer::EnvironmentItem> CMakeBuildConfiguration::userEnvironmentChanges() const
{
    return ProjectExplorer::EnvironmentItem::fromStringList(value("userEnvironmentChanges").toStringList());
}

void CMakeBuildConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    QStringList list = ProjectExplorer::EnvironmentItem::toStringList(diff);
    if (list == value("userEnvironmentChanges"))
        return;
    setValue("userEnvironmentChanges", list);
    emit environmentChanged();
}

QString CMakeBuildConfiguration::buildDirectory() const
{
    QString buildDirectory = value("buildDirectory").toString();
    if (buildDirectory.isEmpty())
        buildDirectory = static_cast<CMakeProject *>(project())->sourceDirectory() + "/qtcreator-build";
    return buildDirectory;
}

QString CMakeBuildConfiguration::buildParser() const
{
    // TODO this is actually slightly wrong, but do i care?
    // this should call toolchain(configuration)
    if (!m_toolChain)
        return QString::null;
    if (m_toolChain->type() == ProjectExplorer::ToolChain::GCC
        //|| m_toolChain->type() == ProjectExplorer::ToolChain::LinuxICC
        || m_toolChain->type() == ProjectExplorer::ToolChain::MinGW) {
        return ProjectExplorer::Constants::BUILD_PARSER_GCC;
    } else if (m_toolChain->type() == ProjectExplorer::ToolChain::MSVC
               || m_toolChain->type() == ProjectExplorer::ToolChain::WINCE) {
        return ProjectExplorer::Constants::BUILD_PARSER_MSVC;
    }
    return QString::null;
}

ProjectExplorer::ToolChain::ToolChainType CMakeBuildConfiguration::toolChainType() const
{
    if (m_toolChain)
        return m_toolChain->type();
    return ProjectExplorer::ToolChain::UNKNOWN;
}

ProjectExplorer::ToolChain *CMakeBuildConfiguration::toolChain() const
{
    return m_toolChain;
}

void CMakeBuildConfiguration::updateToolChain(const QString &compiler)
{
    //qDebug()<<"CodeBlocks Compilername"<<compiler
    ProjectExplorer::ToolChain *newToolChain = 0;
    if (compiler == "gcc") {
#ifdef Q_OS_WIN
        newToolChain = ProjectExplorer::ToolChain::createMinGWToolChain("gcc", QString());
#else
        newToolChain = ProjectExplorer::ToolChain::createGccToolChain("gcc");
#endif
    } else if (compiler == "msvc8") {
        newToolChain = ProjectExplorer::ToolChain::createMSVCToolChain(value("msvcVersion").toString(), false);
    } else {
    }

    if (ProjectExplorer::ToolChain::equals(newToolChain, m_toolChain)) {
        delete newToolChain;
        newToolChain = 0;
    } else {
        delete m_toolChain;
        m_toolChain = newToolChain;
    }
}



