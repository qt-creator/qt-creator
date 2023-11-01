// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosqtversion.h"

#include "iosconstants.h"
#include "iostr.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorer.h>

using namespace ProjectExplorer;

namespace Ios::Internal {

class IosQtVersion : public QtSupport::QtVersion
{
public:
    IosQtVersion();

    bool isValid() const override;
    QString invalidReason() const override;

    Abis detectQtAbis() const override;

    QSet<Utils::Id> availableFeatures() const override;
    QSet<Utils::Id> targetDeviceTypes() const override;

    QString description() const override;
};

IosQtVersion::IosQtVersion() = default;

bool IosQtVersion::isValid() const
{
    if (!QtVersion::isValid())
        return false;
    if (qtAbis().isEmpty())
        return false;
    return true;
}

QString IosQtVersion::invalidReason() const
{
    QString tmp = QtVersion::invalidReason();
    if (tmp.isEmpty() && qtAbis().isEmpty())
        return Tr::tr("Failed to detect the ABIs used by the Qt version.");
    return tmp;
}

Abis IosQtVersion::detectQtAbis() const
{
    Abis abis = QtVersion::detectQtAbis();
    for (int i = 0; i < abis.count(); ++i) {
        abis[i] = Abi(abis.at(i).architecture(),
                      abis.at(i).os(),
                      Abi::GenericFlavor,
                      abis.at(i).binaryFormat(),
                      abis.at(i).wordWidth());
    }
    return abis;
}

QString IosQtVersion::description() const
{
    //: Qt Version is meant for Ios
    return Tr::tr("iOS");
}

QSet<Utils::Id> IosQtVersion::availableFeatures() const
{
    QSet<Utils::Id> features = QtSupport::QtVersion::availableFeatures();
    features.insert(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(QtSupport::Constants::FEATURE_QT_CONSOLE);
    features.remove(QtSupport::Constants::FEATURE_QT_WEBKIT);
    return features;
}

QSet<Utils::Id> IosQtVersion::targetDeviceTypes() const
{
    // iOS Qt version supports ios devices as well as simulator.
    return {Constants::IOS_DEVICE_TYPE, Constants::IOS_SIMULATOR_TYPE};
}


// Factory

IosQtVersionFactory::IosQtVersionFactory()
{
    setQtVersionCreator([] { return new IosQtVersion; });
    setSupportedType(Constants::IOSQT);
    setPriority(90);
    setRestrictionChecker([](const SetupData &setup) {
        return setup.platforms.contains("ios");
    });
}

} // Ios::Internal
