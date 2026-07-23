// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtversions.h"

#include "baseqtversion.h"
#include "qtsupportconstants.h"
#include "qtsupporttr.h"
#include "qtversionfactory.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <remote/remotelinux_constants.h>


#include <utils/algorithm.h>

namespace QtSupport::Internal {

class DesktopQtVersion final : public QtVersion
{
public:
    QString description() const final
    {
        return Tr::tr("Desktop", "Qt Version is meant for the desktop");
    }

    QSet<Utils::Id> availableFeatures() const final
    {
        QSet<Utils::Id> features = QtVersion::availableFeatures();
        features.insert(Constants::FEATURE_DESKTOP);
        features.insert(Constants::FEATURE_QMLPROJECT);
        return features;
    }

    QSet<Utils::Id> targetDeviceTypes() const final
    {
        QSet<Utils::Id> result = {ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE};
        auto targetsOs = [this](const ProjectExplorer::Abi::OS &abi) {
            return Utils::contains(qtAbis(), Utils::equal(&ProjectExplorer::Abi::os, abi));
        };
        if (targetsOs(ProjectExplorer::Abi::LinuxOS))
            result.insert(Remote::Constants::GenericLinuxOsType);
        if (targetsOs(ProjectExplorer::Abi::DarwinOS))
            result.insert(Remote::Constants::GenericMacOsType);
        if (targetsOs(ProjectExplorer::Abi::WindowsOS))
            result.insert(Remote::Constants::GenericWindowsOsType);
        return result;
    }
};

// Factory

class DesktopQtVersionFactory : public QtVersionFactory
{
public:
    DesktopQtVersionFactory()
    {
        setQtVersionCreator([] { return new DesktopQtVersion; });
        setSupportedType(QtSupport::Constants::DESKTOPQT);
        setPriority(0); // Lowest of all, we want to be the fallback
        // No further restrictions. We are the fallback :) so we don't care what kind of qt it is.
    }
};

void setupDesktopQtVersion()
{
    static DesktopQtVersionFactory theDesktopQtVersionFactory;
}

// EmbeddedLinuxQtVersion

const char EMBEDDED_LINUX_QT[] = "RemoteLinux.EmbeddedLinuxQt";

class EmbeddedLinuxQtVersion final : public QtVersion
{
public:
    EmbeddedLinuxQtVersion() = default;

    QString description() const final
    {
        return Tr::tr("Embedded Linux", "Qt Version is used for embedded Linux development");
    }

    QSet<Utils::Id> targetDeviceTypes() const final
    {
        return {Remote::Constants::GenericLinuxOsType};
    }
};

class EmbeddedLinuxQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    EmbeddedLinuxQtVersionFactory()
    {
        setQtVersionCreator([] { return new EmbeddedLinuxQtVersion; });
        setSupportedType(EMBEDDED_LINUX_QT);
        setPriority(10);

        setRestrictionChecker([](const SetupData &) { return false; });
    }
};

void setupEmbeddedLinuxQtVersion()
{
    static EmbeddedLinuxQtVersionFactory theEmbeddedLinuxQtVersionFactory;
}

} // QtSupport::Internal
