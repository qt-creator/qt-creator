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

#include "iosmanager.h"
#include "iosconfigurations.h"
#include "iosrunconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qtsupport/qtkitinformation.h>

using namespace QmakeProjectManager;
using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

/*!
    Returns \c true if the target supports iOS build, \c false otherwise.
*/
bool IosManager::supportsIos(const Target *target)
{
    return qobject_cast<QmakeProject *>(target->project()) && supportsIos(target->kit());
}

/*!
    Returns \c true if the kit supports iOS build, \c false otherwise.
*/
bool IosManager::supportsIos(const Kit *kit)
{
    bool supports = false;
    if (kit) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
        supports = version && version->type() == QLatin1String(Ios::Constants::IOSQT);
    }
    return supports;
}

QString IosManager::resDirForTarget(Target *target)
{
    return target->activeBuildConfiguration()->buildDirectory().toString();
}

} // namespace Internal
} // namespace Ios
