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

QSet<Id> HarmonyOsQtVersion::targetDeviceTypes() const
{
    return {Constants::HARMONYOS_DEVICE_TYPE};
}

QSet<Id> HarmonyOsQtVersion::availableFeatures() const
{
    QSet<Id> features = QtVersion::availableFeatures();
    features.insert(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(QtSupport::Constants::FEATURE_QT_CONSOLE);
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
            return setup.platforms.contains("ohos");
        });
    }
};

void setupHarmonyOsQtVersion()
{
    static HarmonyOsQtVersionFactory theHarmonyOsQtVersionFactory;
}

} // namespace HarmonyOs::Internal
