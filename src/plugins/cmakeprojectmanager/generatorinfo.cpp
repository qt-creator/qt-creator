/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "generatorinfo.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/kitinformation.h>

#include <remotelinux/remotelinux_constants.h>
#include <qnx/qnxconstants.h>

namespace CMakeProjectManager {
namespace Internal {

GeneratorInfo::GeneratorInfo()
    : m_kit(0), m_isNinja(false)
{}

GeneratorInfo::GeneratorInfo(ProjectExplorer::Kit *kit, bool ninja)
    : m_kit(kit), m_isNinja(ninja)
{}

ProjectExplorer::Kit *GeneratorInfo::kit() const
{
    return m_kit;
}

bool GeneratorInfo::isNinja() const {
    return m_isNinja;
}

QByteArray GeneratorInfo::generator() const
{
    if (!m_kit)
        return QByteArray();
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(m_kit);
    ProjectExplorer::Abi targetAbi = tc->targetAbi();
    if (m_isNinja) {
        return "Ninja";
    } else if (targetAbi.os() == ProjectExplorer::Abi::WindowsOS) {
        if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2005Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2008Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2010Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2012Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2013Flavor) {
            return "NMake Makefiles";
        } else if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor) {
            if (Utils::HostOsInfo::isWindowsHost())
                return "MinGW Makefiles";
            else
                return "Unix Makefiles";
        }
    }
    return "Unix Makefiles";
}

QByteArray GeneratorInfo::generatorArgument() const
{
    QByteArray tmp = generator();
    if (tmp.isEmpty())
        return tmp;
    return QByteArray("-GCodeBlocks - ") + tmp;
}

QString GeneratorInfo::displayName() const
{
    if (!m_kit)
        return QString();
    if (m_isNinja)
        return tr("Ninja (%1)").arg(m_kit->displayName());
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(m_kit);
    ProjectExplorer::Abi targetAbi = tc->targetAbi();
    if (targetAbi.os() == ProjectExplorer::Abi::WindowsOS) {
        if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2005Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2008Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2010Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2012Flavor
                || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2013Flavor) {
            return tr("NMake Generator (%1)").arg(m_kit->displayName());
        } else if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor) {
            if (Utils::HostOsInfo::isWindowsHost())
                return tr("MinGW Generator (%1)").arg(m_kit->displayName());
            else
                return tr("Unix Generator (%1)").arg(m_kit->displayName());
        }
    } else {
        // Non windows
        return tr("Unix Generator (%1)").arg(m_kit->displayName());
    }
    return QString();
}

QList<GeneratorInfo> GeneratorInfo::generatorInfosFor(ProjectExplorer::Kit *k, Ninja n, bool preferNinja, bool hasCodeBlocks)
{
    QList<GeneratorInfo> results;
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
    if (!tc)
        return results;
    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k);
    if (deviceType !=  ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
            && deviceType != RemoteLinux::Constants::GenericLinuxOsType
            && deviceType != Qnx::Constants::QNX_QNX_OS_TYPE
            && deviceType != Qnx::Constants::QNX_BB_OS_TYPE)
        return results;
    ProjectExplorer::Abi targetAbi = tc->targetAbi();
    if (n != ForceNinja) {
        if (targetAbi.os() == ProjectExplorer::Abi::WindowsOS) {
            if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2005Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2008Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2010Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2012Flavor
                    || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2013Flavor) {
                if (hasCodeBlocks)
                    results << GeneratorInfo(k);
            } else if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor) {
                results << GeneratorInfo(k);
            }
        } else {
            // Non windows
            results << GeneratorInfo(k);
        }
    }
    if (n != NoNinja) {
        if (preferNinja)
            results.prepend(GeneratorInfo(k, true));
        else
            results.append(GeneratorInfo(k, true));
    }
    return results;
}

} // namespace Internal
} // namespace CMakeProjectManager
