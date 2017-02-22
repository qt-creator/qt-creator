/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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
    QList<Abi> abis = qtAbisFromLibrary(qtCorePaths());
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

QSet<Core::Id> AndroidQtVersion::availableFeatures() const
{
    QSet<Core::Id> features = QtSupport::BaseQtVersion::availableFeatures();
    features.insert(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(QtSupport::Constants::FEATURE_QT_CONSOLE);
    features.remove(QtSupport::Constants::FEATURE_QT_WEBKIT);
    return features;
}

QSet<Core::Id> AndroidQtVersion::targetDeviceTypes() const
{
    return {Constants::ANDROID_DEVICE_TYPE};
}
