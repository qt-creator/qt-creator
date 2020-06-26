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

#include "qtversions.h"

#include "baseqtversion.h"
#include "qtsupportconstants.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <remotelinux/remotelinux_constants.h>

#include <coreplugin/featureprovider.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFileInfo>

namespace QtSupport {
namespace Internal {

class DesktopQtVersion : public BaseQtVersion
{
public:
    DesktopQtVersion() = default;

    QStringList warningReason() const override;

    QString description() const override;

    QSet<Utils::Id> availableFeatures() const override;
    QSet<Utils::Id> targetDeviceTypes() const override;
};

QStringList DesktopQtVersion::warningReason() const
{
    QStringList ret = BaseQtVersion::warningReason();
    if (qtVersion() >= QtVersionNumber(5, 0, 0)) {
        if (qmlsceneCommand().isEmpty())
            ret << QCoreApplication::translate("QtVersion", "No qmlscene installed.");
    }
    return ret;
}

QString DesktopQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Desktop", "Qt Version is meant for the desktop");
}

QSet<Utils::Id> DesktopQtVersion::availableFeatures() const
{
    QSet<Utils::Id> features = BaseQtVersion::availableFeatures();
    features.insert(Constants::FEATURE_DESKTOP);
    features.insert(Constants::FEATURE_QMLPROJECT);
    return features;
}

QSet<Utils::Id> DesktopQtVersion::targetDeviceTypes() const
{
    QSet<Utils::Id> result = {ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE};
    if (Utils::contains(qtAbis(), [](const ProjectExplorer::Abi a) { return a.os() == ProjectExplorer::Abi::LinuxOS; }))
        result.insert(RemoteLinux::Constants::GenericLinuxOsType);
    return result;
}

// Factory

DesktopQtVersionFactory::DesktopQtVersionFactory()
{
    setQtVersionCreator([] { return new DesktopQtVersion; });
    setSupportedType(QtSupport::Constants::DESKTOPQT);
    setPriority(0); // Lowest of all, we want to be the fallback
    // No further restrictions. We are the fallback :) so we don't care what kind of qt it is.
}


// EmbeddedLinuxQtVersion

const char EMBEDDED_LINUX_QT[] = "RemoteLinux.EmbeddedLinuxQt";

class EmbeddedLinuxQtVersion : public BaseQtVersion
{
public:
    EmbeddedLinuxQtVersion() = default;

    QString description() const override
    {
        return QCoreApplication::translate("QtVersion", "Embedded Linux",
                                           "Qt Version is used for embedded Linux development");
    }

    QSet<Utils::Id> targetDeviceTypes() const override
    {
        return {RemoteLinux::Constants::GenericLinuxOsType};
    }
};

EmbeddedLinuxQtVersionFactory::EmbeddedLinuxQtVersionFactory()
{
    setQtVersionCreator([] { return new EmbeddedLinuxQtVersion; });
    setSupportedType(EMBEDDED_LINUX_QT);
    setPriority(10);

    setRestrictionChecker([](const SetupData &) { return false; });
}

} // Internal
} // QtSupport
