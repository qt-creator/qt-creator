/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "iosqtversion.h"
#include "iosconstants.h"
#include "iosconfigurations.h"
#include "iosmanager.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorer.h>

using namespace Ios::Internal;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

IosQtVersion::IosQtVersion()
    : QtSupport::BaseQtVersion()
{
}

IosQtVersion::IosQtVersion(const Utils::FileName &path, bool isAutodetected,
                           const QString &autodetectionSource)
    : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource)
{
    setUnexpandedDisplayName(defaultUnexpandedDisplayName(path, false));
}

IosQtVersion *IosQtVersion::clone() const
{
    return new IosQtVersion(*this);
}

QString IosQtVersion::type() const
{
    return QLatin1String(Constants::IOSQT);
}

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

QList<Abi> IosQtVersion::detectQtAbis() const
{
    QList<Abi> abis = qtAbisFromLibrary(qtCorePaths(versionInfo(), qtVersionString()));
    for (int i = 0; i < abis.count(); ++i) {
        abis[i] = Abi(abis.at(i).architecture(),
                      abis.at(i).os(),
                      Abi::GenericMacFlavor,
                      abis.at(i).binaryFormat(),
                      abis.at(i).wordWidth());
    }
    return abis;
}

void IosQtVersion::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    Q_UNUSED(k);
    Q_UNUSED(env);
}

QString IosQtVersion::description() const
{
    //: Qt Version is meant for Ios
    return tr("iOS");
}

Core::FeatureSet IosQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QtSupport::BaseQtVersion::availableFeatures();
    features |= Core::FeatureSet(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(Core::Feature(QtSupport::Constants::FEATURE_QT_CONSOLE));
    features.remove(Core::Feature(QtSupport::Constants::FEATURE_QT_WEBKIT));
    return features;
}

QString IosQtVersion::platformName() const
{
    return QLatin1String(QtSupport::Constants::IOS_PLATFORM);
}

QString IosQtVersion::platformDisplayName() const
{
    return QLatin1String(QtSupport::Constants::IOS_PLATFORM_TR);
}
