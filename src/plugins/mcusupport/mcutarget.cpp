/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "mcutarget.h"
#include "mcupackage.h"
#include "mcukitmanager.h"
#include "mcusupportplugin.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace McuSupport::Internal {

McuTarget::McuTarget(const QVersionNumber &qulVersion,
                     const Platform &platform,
                     OS os,
                     const QVector<McuAbstractPackage *> &packages,
                     const McuToolChainPackage *toolChainPackage,
                     int colorDepth)
    : m_qulVersion(qulVersion)
    , m_platform(platform)
    , m_os(os)
    , m_packages(packages)
    , m_toolChainPackage(toolChainPackage)
    , m_colorDepth(colorDepth)
{}

const QVector<McuAbstractPackage *> &McuTarget::packages() const
{
    return m_packages;
}

const McuToolChainPackage *McuTarget::toolChainPackage() const
{
    return m_toolChainPackage;
}

McuTarget::OS McuTarget::os() const
{
    return m_os;
}

const McuTarget::Platform &McuTarget::platform() const
{
    return m_platform;
}

bool McuTarget::isValid() const
{
    return Utils::allOf(packages(), [](McuAbstractPackage *package) {
        package->updateStatus();
        return package->isValidStatus();
    });
}

void McuTarget::printPackageProblems() const
{
    for (auto package : packages()) {
        package->updateStatus();
        if (!package->isValidStatus())
            printMessage(tr("Error creating kit for target %1, package %2: %3")
                             .arg(McuKitManager::kitName(this),
                                  package->label(),
                                  package->statusText()),
                         true);
        if (package->status() == McuAbstractPackage::Status::ValidPackageMismatchedVersion)
            printMessage(tr("Warning creating kit for target %1, package %2: %3")
                             .arg(McuKitManager::kitName(this),
                                  package->label(),
                                  package->statusText()),
                         false);
    }
}

const QVersionNumber &McuTarget::qulVersion() const
{
    return m_qulVersion;
}

int McuTarget::colorDepth() const
{
    return m_colorDepth;
}

} // namespace McuSupport::Internal
