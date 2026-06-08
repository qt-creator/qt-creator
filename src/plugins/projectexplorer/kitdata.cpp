// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitdata.h"

#include "devicesupport/idevicefactory.h"
#include "projectexplorerconstants.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace ProjectExplorer {

QString KitData::unexpandedDisplayName() const
{
    return m_unexpandedDisplayName.value();
}

bool KitData::operator==(const KitData &other) const
{
    return m_detectionSource == other.m_detectionSource
        && m_data == other.m_data
        && m_iconPath == other.m_iconPath
        && m_deviceTypeForIcon == other.m_deviceTypeForIcon
        && m_unexpandedDisplayName == other.m_unexpandedDisplayName
        && m_fileSystemFriendlyName == other.m_fileSystemFriendlyName
        && m_relevantAspects == other.m_relevantAspects
        && m_irrelevantAspects == other.m_irrelevantAspects
        && m_mutable == other.m_mutable;
    // Note: m_id and m_sticky are intentionally left out:
    // - m_id identifies a registered kit but is not user-editable; dirty-detection
    //   always compares the committed and volatile copies of the same row, so both
    //   have the same id anyway.
    // - m_sticky: Kit::setSticky() calls kitUpdated() directly and never goes through
    //   the working copy, so dirty-detection doesn't need to track it.
}

QIcon KitData::icon() const
{
    if (!m_deviceTypeForIcon.isValid() && !m_iconPath.isEmpty() && m_iconPath.exists())
        return QIcon(m_iconPath.toFSPathString());
    if (m_deviceTypeForIcon.isValid()) {
        const QIcon deviceTypeIcon = IDeviceFactory::iconForDeviceType(m_deviceTypeForIcon);
        if (!deviceTypeIcon.isNull())
            return deviceTypeIcon;
    }
    // FIXME: Kit::icon() uses RunDeviceTypeKitAspect::deviceTypeId() as a fallback here,
    // so working copies of kits with a non-desktop run device show the wrong icon.
    return IDeviceFactory::iconForDeviceType(Constants::DESKTOP_DEVICE_TYPE);
}

} // namespace ProjectExplorer
