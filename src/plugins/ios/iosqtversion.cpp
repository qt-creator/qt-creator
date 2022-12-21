// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosqtversion.h"
#include "iosconstants.h"
#include "iosconfigurations.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorer.h>

using namespace Ios::Internal;
using namespace ProjectExplorer;

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
        return tr("Failed to detect the ABIs used by the Qt version.");
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
    return tr("iOS");
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
