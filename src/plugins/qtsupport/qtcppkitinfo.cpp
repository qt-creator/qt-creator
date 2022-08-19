// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qtcppkitinfo.h"

#include "baseqtversion.h"
#include "qtkitinformation.h"

namespace QtSupport {

CppKitInfo::CppKitInfo(ProjectExplorer::Kit *kit)
    : ProjectExplorer::KitInfo(kit)
{
    if (kit && (qtVersion = QtKitAspect::qtVersion(kit))) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5, 0, 0))
            projectPartQtVersion = Utils::QtMajorVersion::Qt4;
        else if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(6, 0, 0))
            projectPartQtVersion = Utils::QtMajorVersion::Qt5;
        else
            projectPartQtVersion = Utils::QtMajorVersion::Qt6;
    }
}

} // namespace QtSupport
