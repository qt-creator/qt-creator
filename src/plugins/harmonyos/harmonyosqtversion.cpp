// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosqtversion.h"

#include "harmonyosconstants.h"
#include "harmonyostr.h"

#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionfactory.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace HarmonyOs::Internal {

HarmonyOsQtVersion::HarmonyOsQtVersion() = default;

QString HarmonyOsQtVersion::description() const
{
    //: Qt Version is meant for HarmonyOS
    return Tr::tr("HarmonyOS");
}

Abis HarmonyOsQtVersion::detectQtAbis() const
{
    Abis abis = QtVersion::detectQtAbis();
    if (abis.isEmpty())
        abis = Abi::abisOfBinary(libraryPath().pathAppended("libQt6Core.so"));

    Abis harmonyOsAbis;
    for (const Abi &abi : abis) {
        if (abi.architecture() != Abi::UnknownArchitecture) {
            harmonyOsAbis.append(Abi(abi.architecture(), Abi::LinuxOS,
                                     Abi::OpenHarmonyLinuxFlavor, Abi::ElfFormat,
                                     abi.wordWidth()));
        }
    }
    return harmonyOsAbis;
}

QSet<Id> HarmonyOsQtVersion::targetDeviceTypes() const
{
    return {Constants::HARMONYOS_DEVICE_TYPE, Constants::HARMONYOS_BUILD_DEVICE_TYPE};
}

QSet<Id> HarmonyOsQtVersion::availableFeatures() const
{
    QSet<Id> features = QtVersion::availableFeatures();
    features.insert(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(QtSupport::Constants::FEATURE_QT_WEBKIT);
    return features;
}

class HarmonyOsQtVersionFactory final : public QtVersionFactory
{
public:
    HarmonyOsQtVersionFactory()
    {
        setQtVersionCreator([] { return new HarmonyOsQtVersion; });
        setSupportedType(Constants::HARMONYOS_QT_TYPE);
        setPriority(90);
        setRestrictionChecker([](const SetupData &setup) {
            // The ohos mkspec bails out unless NATIVE_OHOS_SDK is set in the environment,
            // which leaves QMAKE_PLATFORM empty. The mkspec name is read from qmake's
            // QMAKE_XSPEC and stays available, so fall back to it.
            return setup.platforms.contains("ohos") || setup.mkspec.startsWith("ohos-");
        });
    }
};

void setupHarmonyOsQtVersion()
{
    static HarmonyOsQtVersionFactory theHarmonyOsQtVersionFactory;
}

} // namespace HarmonyOs::Internal
