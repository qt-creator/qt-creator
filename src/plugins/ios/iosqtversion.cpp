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
    if (!BaseQtVersion::isValid())
        return false;
    if (qtAbis().isEmpty())
        return false;
    return true;
}

QString IosQtVersion::invalidReason() const
{
    QString tmp = BaseQtVersion::invalidReason();
    if (tmp.isEmpty() && qtAbis().isEmpty())
        return tr("Failed to detect the ABIs used by the Qt version.");
    return tmp;
}

Abis IosQtVersion::detectQtAbis() const
{
    Abis abis = BaseQtVersion::detectQtAbis();
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
    QSet<Utils::Id> features = QtSupport::BaseQtVersion::availableFeatures();
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
