// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtversions.h"

#include "baseqtversion.h"
#include "qtsupportconstants.h"
#include "qtsupporttr.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <remotelinux/remotelinux_constants.h>

#include <coreplugin/featureprovider.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

namespace QtSupport::Internal {

class DesktopQtVersion : public QtVersion
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
    QStringList ret = QtVersion::warningReason();
    if (qtVersion() >= QVersionNumber(5, 0, 0)) {
        if (qmlRuntimeFilePath().isEmpty())
            ret << Tr::tr("No QML utility installed.");
    }
    return ret;
}

QString DesktopQtVersion::description() const
{
    return Tr::tr("Desktop", "Qt Version is meant for the desktop");
}

QSet<Utils::Id> DesktopQtVersion::availableFeatures() const
{
    QSet<Utils::Id> features = QtVersion::availableFeatures();
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

class EmbeddedLinuxQtVersion : public QtVersion
{
public:
    EmbeddedLinuxQtVersion() = default;

    QString description() const override
    {
        return Tr::tr("Embedded Linux", "Qt Version is used for embedded Linux development");
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

} // QtSupport::Internal
