/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberryqtversion.h"

#include "qnxutils.h"
#include "qnxconstants.h"

#include <coreplugin/featureprovider.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <qtsupport/qtsupportconstants.h>

#include <QFileInfo>
#include <QTextStream>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const QLatin1String NndkEnvFile("ndkEnvFile");
}

BlackBerryQtVersion::BlackBerryQtVersion()
    : QnxAbstractQtVersion()
{
}

BlackBerryQtVersion::BlackBerryQtVersion(QnxArchitecture arch, const Utils::FileName &path, bool isAutoDetected, const QString &autoDetectionSource, const QString &sdkPath)
    : QnxAbstractQtVersion(arch, path, isAutoDetected, autoDetectionSource)
{
    if (!sdkPath.isEmpty()) {
        if (QFileInfo(sdkPath).isDir()) {
            setSdkPath(sdkPath);
        } else {
            m_ndkEnvFile = sdkPath;
            setSdkPath(QFileInfo(sdkPath).absolutePath());
        }

    } else {
        setDefaultSdkPath();
    }
}

BlackBerryQtVersion::~BlackBerryQtVersion()
{

}

BlackBerryQtVersion *BlackBerryQtVersion::clone() const
{
    return new BlackBerryQtVersion(*this);
}

QString BlackBerryQtVersion::type() const
{
    return QLatin1String(Constants::QNX_BB_QT);
}

QString BlackBerryQtVersion::description() const
{
    return tr("BlackBerry %1", "Qt Version is meant for BlackBerry").arg(archString());
}

QVariantMap BlackBerryQtVersion::toMap() const
{
    QVariantMap result = QnxAbstractQtVersion::toMap();
    result.insert(NndkEnvFile, m_ndkEnvFile);
    return result;
}

void BlackBerryQtVersion::fromMap(const QVariantMap &map)
{
    QnxAbstractQtVersion::fromMap(map);
    m_ndkEnvFile = map.value(NndkEnvFile).toString();
}

QList<Utils::EnvironmentItem> BlackBerryQtVersion::environment() const
{
    QTC_CHECK(!sdkPath().isEmpty());
    if (sdkPath().isEmpty())
        return QList<Utils::EnvironmentItem>();

    QString envFile = m_ndkEnvFile.isEmpty() ? QnxUtils::envFilePath(sdkPath()) : m_ndkEnvFile;
    QList<Utils::EnvironmentItem> env = QnxUtils::qnxEnvironmentFromEnvFile(envFile);

    // BB NDK Host is having qmake executable which is using qt.conf file to specify
    // base information. The qt.conf file is using 'CPUVARDIR' environment variable
    // to provide correct information for both x86 and armle-v7 architectures.
    // BlackBerryQtVersion represents as specific environment for each Qt4/Qt5
    // and x86/armle-v7 combination. Therefore we need to explicitly specify
    // CPUVARDIR to match expected architecture() otherwise qmake environment is
    // always resolved to be for armle-v7 architecture only as it is specified
    // BB NDK environment file.

    env.append(Utils::EnvironmentItem(QLatin1String("CPUVARDIR"),
            architecture() == X86 ? QLatin1String("x86") : QLatin1String("armle-v7")));

    return env;
}

void BlackBerryQtVersion::setDefaultSdkPath()
{
    QHash<QString, QString> info = versionInfo();
    QString qtHostPrefix;
    if (info.contains(QLatin1String("QT_HOST_PREFIX")))
        qtHostPrefix = info.value(QLatin1String("QT_HOST_PREFIX"));
    else
        return;

    if (QnxUtils::isValidNdkPath(qtHostPrefix))
        setSdkPath(qtHostPrefix);
}

Core::FeatureSet BlackBerryQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QnxAbstractQtVersion::availableFeatures();
    features |= Core::FeatureSet(Constants::QNX_BB_FEATURE);
    features.remove(Core::Feature(QtSupport::Constants::FEATURE_QT_CONSOLE));
    features.remove(Core::Feature(QtSupport::Constants::FEATURE_QT_WEBKIT));
    return features;
}

QString BlackBerryQtVersion::platformName() const
{
    return QLatin1String(Constants::QNX_BB_PLATFORM_NAME);
}

QString BlackBerryQtVersion::platformDisplayName() const
{
    return tr("BlackBerry");
}

QString BlackBerryQtVersion::sdkDescription() const
{
    return tr("BlackBerry Native SDK:");
}
