/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "projectpartbuilder.h"

#include "projectinfo.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

namespace CppTools {

class ToolChainImpl : public ToolChainInterface
{
public:
    ToolChainImpl(ProjectExplorer::ToolChain &toolChain,
                  const ProjectExplorer::Kit *kit,
                  const QStringList &commandLineFlags)
        : m_toolChain(toolChain)
        , m_kit(kit)
        , m_commandLineFlags(commandLineFlags)
    {
    }

    Core::Id type() const override
    {
        return m_toolChain.typeId();
    }

    bool isMsvc2015Toolchain() const override
    {
        return m_toolChain.targetAbi().osFlavor() == ProjectExplorer::Abi::WindowsMsvc2015Flavor;
    }

    unsigned wordWidth() const override
    {
        return m_toolChain.targetAbi().wordWidth();
    }

    QString targetTriple() const override
    {
        if (type() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
            return QLatin1String("i686-pc-windows-msvc");

        return m_toolChain.originalTargetTriple();
    }

    QByteArray predefinedMacros() const override
    {
        return m_toolChain.predefinedMacros(m_commandLineFlags);
    }

    QList<ProjectExplorer::HeaderPath> systemHeaderPaths() const override
    {
        return m_toolChain.systemHeaderPaths(
                    m_commandLineFlags,
                    ProjectExplorer::SysRootKitInformation::sysRoot(m_kit));
    }

    ProjectExplorer::WarningFlags warningFlags() const override
    {
        return m_toolChain.warningFlags(m_commandLineFlags);
    }

    ProjectExplorer::ToolChain::CompilerFlags compilerFlags() const override
    {
        return m_toolChain.compilerFlags(m_commandLineFlags);
    }

private:
    ProjectExplorer::ToolChain &m_toolChain;
    const ProjectExplorer::Kit *m_kit = nullptr;
    const QStringList m_commandLineFlags;
};

class ProjectImpl : public ProjectInterface
{
public:
    ProjectImpl(ProjectExplorer::Project &project)
        : m_project(project)
    {
        if (ProjectExplorer::Target *activeTarget = m_project.activeTarget())
            m_kit = activeTarget->kit();
    }

    QString displayName() const override
    {
        return m_project.displayName();
    }

    QString projectFilePath() const override
    {
        return m_project.projectFilePath().toString();
    }

    ToolChainInterfacePtr toolChain(Core::Id language,
                                    const QStringList &commandLineFlags) const override
    {
        using namespace ProjectExplorer;

        if (ProjectExplorer::ToolChain *t = ToolChainKitInformation::toolChain(m_kit, language))
            return ToolChainInterfacePtr(new ToolChainImpl(*t, m_kit, commandLineFlags));

        return ToolChainInterfacePtr();
    }

private:
    ProjectExplorer::Project &m_project;
    ProjectExplorer::Kit *m_kit = nullptr;
};

ProjectPartBuilder::ProjectPartBuilder(ProjectInfo &projectInfo)
    : BaseProjectPartBuilder(new ProjectImpl(*projectInfo.project().data()), projectInfo)
{
}

} // namespace CppTools
