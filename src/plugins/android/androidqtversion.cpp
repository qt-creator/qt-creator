/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidqtversion.h"
#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androidmanager.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>

#include <proparser/profileevaluator.h>

using namespace Android::Internal;
using namespace ProjectExplorer;

AndroidQtVersion::AndroidQtVersion()
    : QtSupport::BaseQtVersion()
{
}

AndroidQtVersion::AndroidQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource)
{
    setUnexpandedDisplayName(defaultUnexpandedDisplayName(path, false));
}

AndroidQtVersion *AndroidQtVersion::clone() const
{
    return new AndroidQtVersion(*this);
}

QString AndroidQtVersion::type() const
{
    return QLatin1String(Constants::ANDROIDQT);
}

bool AndroidQtVersion::isValid() const
{
    if (!BaseQtVersion::isValid())
        return false;
    if (qtAbis().isEmpty())
        return false;
    return true;
}

QString AndroidQtVersion::invalidReason() const
{
    QString tmp = BaseQtVersion::invalidReason();
    if (tmp.isEmpty() && qtAbis().isEmpty())
        return tr("Failed to detect the ABIs used by the Qt version.");
    return tmp;
}

QList<Abi> AndroidQtVersion::detectQtAbis() const
{
    QList<Abi> abis = qtAbisFromLibrary(qtCorePaths(versionInfo(), qtVersionString()));
    for (int i = 0; i < abis.count(); ++i) {
        abis[i] = Abi(abis.at(i).architecture(),
                      abis.at(i).os(),
                      Abi::AndroidLinuxFlavor,
                      abis.at(i).binaryFormat(),
                      abis.at(i).wordWidth());
    }
    return abis;
}

void AndroidQtVersion::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    Q_UNUSED(k);
    // this env vars are used by qmake mkspecs to generate makefiles (check QTDIR/mkspecs/android-g++/qmake.conf for more info)
    env.set(QLatin1String("ANDROID_NDK_HOST"), AndroidConfigurations::currentConfig().toolchainHost());
    env.set(QLatin1String("ANDROID_NDK_ROOT"), AndroidConfigurations::currentConfig().ndkLocation().toUserOutput());
}

Utils::Environment AndroidQtVersion::qmakeRunEnvironment() const
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    env.set(QLatin1String("ANDROID_NDK_ROOT"), AndroidConfigurations::currentConfig().ndkLocation().toUserOutput());
    return env;
}

QString AndroidQtVersion::description() const
{
    //: Qt Version is meant for Android
    return tr("Android");
}

QString AndroidQtVersion::targetArch() const
{
    ensureMkSpecParsed();
    return m_targetArch;
}

void AndroidQtVersion::parseMkSpec(ProFileEvaluator *evaluator) const
{
    m_targetArch = evaluator->value(QLatin1String("ANDROID_TARGET_ARCH"));
    BaseQtVersion::parseMkSpec(evaluator);
}

Core::FeatureSet AndroidQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QtSupport::BaseQtVersion::availableFeatures();
    features |= Core::FeatureSet(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(Core::Feature(QtSupport::Constants::FEATURE_QT_CONSOLE));
    features.remove(Core::Feature(QtSupport::Constants::FEATURE_QT_WEBKIT));
    return features;
}

QString AndroidQtVersion::platformName() const
{
    return QLatin1String(QtSupport::Constants::ANDROID_PLATFORM);
}

QString AndroidQtVersion::platformDisplayName() const
{
    return QLatin1String(QtSupport::Constants::ANDROID_PLATFORM_TR);
}
