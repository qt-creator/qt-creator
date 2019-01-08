/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "cppkitinfo.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

namespace CppTools {

using namespace ProjectExplorer;

KitInfo::KitInfo(Project *project)
{
    // Kit
    if (Target *target = project->activeTarget())
        kit = target->kit();
    else
        kit = KitManager::defaultKit();

    // Toolchains
    if (kit) {
        cToolChain = ToolChainKitInformation::toolChain(kit, Constants::C_LANGUAGE_ID);
        cxxToolChain = ToolChainKitInformation::toolChain(kit, Constants::CXX_LANGUAGE_ID);
    }

    // Sysroot
    sysRootPath = ProjectExplorer::SysRootKitInformation::sysRoot(kit).toString();
}

bool KitInfo::isValid() const
{
    return kit;
}

} // namespace CppTools
