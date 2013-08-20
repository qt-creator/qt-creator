/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidqbspropertyprovider.h"

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidgdbserverkitinformation.h"
#include "androidtoolchain.h"

#include <qbsprojectmanager/qbsconstants.h>

namespace Android {
namespace Internal {

// QBS Android specific settings:
const QLatin1String CPP_ANDROID_SDK_PATH("cpp.androidSdkPath");
const QLatin1String CPP_ANDROID_NDK_PATH("cpp.androidNdkPath");
const QLatin1String CPP_ANDROID_TOOLCHAIN_VERSION("cpp.androidToolchainVersion");
const QLatin1String CPP_ANDROID_TOOLCHAIN_HOST("cpp.androidToolchainHost");
const QLatin1String CPP_ANDROID_TOOLCHAIN_PREFIX("cpp.androidToolchainPrefix");
const QLatin1String CPP_ANDROID_GDBSERVER("cpp.androidGdbServer");

bool AndroidQBSPropertyProvider::canHandle(const ProjectExplorer::Kit *kit) const
{
    return AndroidGdbServerKitInformation::isAndroidKit(kit);
}

QVariantMap AndroidQBSPropertyProvider::properties(const ProjectExplorer::Kit *kit, const QVariantMap &defaultData) const
{
    Q_ASSERT(AndroidGdbServerKitInformation::isAndroidKit(kit));

    QVariantMap qbsProperties = defaultData;
    QStringList targetOSs(defaultData[QLatin1String(QbsProjectManager::Constants::QBS_TARGETOS)].toStringList());
    if (!targetOSs.contains(QLatin1String("android")))
        qbsProperties[QLatin1String(QbsProjectManager::Constants::QBS_TARGETOS)] = QStringList() << QLatin1String("android")
                                                     << targetOSs;

    const AndroidConfig &config = AndroidConfigurations::instance().config();
    AndroidToolChain *tc = static_cast<AndroidToolChain*>(ProjectExplorer::ToolChainKitInformation::toolChain(kit));
    qbsProperties[CPP_ANDROID_SDK_PATH] = config.sdkLocation.toString();
    qbsProperties[CPP_ANDROID_NDK_PATH] = config.ndkLocation.toString();
    qbsProperties[CPP_ANDROID_TOOLCHAIN_VERSION] = tc->ndkToolChainVersion();
    qbsProperties[CPP_ANDROID_TOOLCHAIN_HOST] = config.toolchainHost;
    qbsProperties[CPP_ANDROID_TOOLCHAIN_PREFIX] = AndroidConfigurations::toolchainPrefix(tc->targetAbi().architecture());
    qbsProperties[CPP_ANDROID_GDBSERVER] = tc->suggestedGdbServer().toString();
    // TODO: Find a way to extract ANDROID_ARCHITECTURE from Qt mkspec
//            qbsProperties[QbsProjectManager::Constants::QBS_ARCHITECTURE] = ...

    return qbsProperties;
}

} // namespace Internal
} // namespace Android
