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

#include "winrtqtversion.h"

#include "winrtconstants.h"

#include <coreplugin/featureprovider.h>
#include <qtsupport/qtsupportconstants.h>

namespace WinRt {
namespace Internal {

WinRtQtVersion::WinRtQtVersion(const Utils::FileName &path, bool isAutodetected,
        const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource)
{
    setUnexpandedDisplayName(defaultUnexpandedDisplayName(path, false));
}

QtSupport::BaseQtVersion *WinRtQtVersion::clone() const
{
    return new WinRtQtVersion(*this);
}

QString WinRtQtVersion::type() const
{
    return QLatin1String(Constants::WINRT_WINRTQT);
}

QString WinRtQtVersion::description() const
{
    return tr("Windows Runtime");
}

QSet<Core::Id> WinRtQtVersion::availableFeatures() const
{
    QSet<Core::Id> features = QtSupport::BaseQtVersion::availableFeatures();
    features.insert(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(QtSupport::Constants::FEATURE_QT_CONSOLE);
    features.remove(Core::Id::versionedId(QtSupport::Constants::FEATURE_QT_QUICK_CONTROLS_PREFIX, 1));
    features.remove(QtSupport::Constants::FEATURE_QT_WEBKIT);
    return features;
}

QList<ProjectExplorer::Abi> WinRtQtVersion::detectQtAbis() const
{
    return qtAbisFromLibrary(qtCorePaths());
}

QSet<Core::Id> WinRtQtVersion::targetDeviceTypes() const
{
    return { Constants::WINRT_DEVICE_TYPE_LOCAL, Constants::WINRT_DEVICE_TYPE_EMULATOR };
}

} // Internal
} // WinRt
