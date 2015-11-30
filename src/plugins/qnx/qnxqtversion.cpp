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

#include "qnxqtversion.h"

#include "qnxbaseqtconfigwidget.h"
#include "qnxconstants.h"
#include "qnxutils.h"

#include <coreplugin/featureprovider.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDir>

namespace Qnx {
namespace Internal {

static char SDK_PATH_KEY[] = "SDKPath";
static char ARCH_KEY[] = "Arch";

QnxQtVersion::QnxQtVersion() : m_arch(UnknownArch)
{ }

QnxQtVersion::QnxQtVersion(QnxArchitecture arch, const Utils::FileName &path, bool isAutoDetected,
                           const QString &autoDetectionSource) :
    QtSupport::BaseQtVersion(path, isAutoDetected, autoDetectionSource),
    m_arch(arch)
{
    setUnexpandedDisplayName(defaultUnexpandedDisplayName(path, false));
}

QnxQtVersion *QnxQtVersion::clone() const
{
    return new QnxQtVersion(*this);
}

QString QnxQtVersion::type() const
{
    return QLatin1String(Constants::QNX_QNX_QT);
}

QString QnxQtVersion::description() const
{
    //: Qt Version is meant for QNX
    return QCoreApplication::translate("Qnx::Internal::QnxQtVersion", "QNX %1").arg(archString());
}

Core::FeatureSet QnxQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QtSupport::BaseQtVersion::availableFeatures();
    features |= Core::FeatureSet(Constants::QNX_QNX_FEATURE);
    features.remove(Core::Feature(QtSupport::Constants::FEATURE_QT_CONSOLE));
    features.remove(Core::Feature(QtSupport::Constants::FEATURE_QT_WEBKIT));
    return features;
}

QString QnxQtVersion::platformName() const
{
    return QString::fromLatin1(Constants::QNX_QNX_PLATFORM_NAME);
}

QString QnxQtVersion::platformDisplayName() const
{
    return QCoreApplication::translate("Qnx::Internal::QnxQtVersion", "QNX");
}

QString QnxQtVersion::qnxHost() const
{
    if (!m_environmentUpToDate)
        updateEnvironment();

    foreach (const Utils::EnvironmentItem &item, m_qnxEnv) {
        if (item.name == QLatin1String(Constants::QNX_HOST_KEY))
            return item.value;
    }

    return QString();
}

QString QnxQtVersion::qnxTarget() const
{
    if (!m_environmentUpToDate)
        updateEnvironment();

    foreach (const Utils::EnvironmentItem &item, m_qnxEnv) {
        if (item.name == QLatin1String(Constants::QNX_TARGET_KEY))
            return item.value;
    }

    return QString();
}

QnxArchitecture QnxQtVersion::architecture() const
{
    return m_arch;
}

QString QnxQtVersion::archString() const
{
    switch (m_arch) {
    case X86:
        return QLatin1String("x86");
    case ArmLeV7:
        return QLatin1String("ARMle-v7");
    case UnknownArch:
        return QString();
    }
    return QString();
}

QVariantMap QnxQtVersion::toMap() const
{
    QVariantMap result = BaseQtVersion::toMap();
    result.insert(QLatin1String(SDK_PATH_KEY), sdkPath());
    result.insert(QLatin1String(ARCH_KEY), m_arch);
    return result;
}

void QnxQtVersion::fromMap(const QVariantMap &map)
{
    BaseQtVersion::fromMap(map);
    setSdkPath(QDir::fromNativeSeparators(map.value(QLatin1String(SDK_PATH_KEY)).toString()));
    m_arch = static_cast<QnxArchitecture>(map.value(QLatin1String(ARCH_KEY), UnknownArch).toInt());
}

QList<ProjectExplorer::Abi> QnxQtVersion::detectQtAbis() const
{
    ensureMkSpecParsed();
    return qtAbisFromLibrary(qtCorePaths(versionInfo(), qtVersionString()));
}

void QnxQtVersion::addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const
{
    QtSupport::BaseQtVersion::addToEnvironment(k, env);
    updateEnvironment();
    env.modify(m_qnxEnv);

    env.prependOrSetLibrarySearchPath(versionInfo().value(QLatin1String("QT_INSTALL_LIBS")));
}

Utils::Environment QnxQtVersion::qmakeRunEnvironment() const
{
    if (!sdkPath().isEmpty())
        updateEnvironment();

    Utils::Environment env = Utils::Environment::systemEnvironment();
    env.modify(m_qnxEnv);

    return env;
}

QtSupport::QtConfigWidget *QnxQtVersion::createConfigurationWidget() const
{
    return new QnxBaseQtConfigWidget(const_cast<QnxQtVersion *>(this));
}

bool QnxQtVersion::isValid() const
{
    return QtSupport::BaseQtVersion::isValid() && !sdkPath().isEmpty();
}

QString QnxQtVersion::invalidReason() const
{
    if (sdkPath().isEmpty())
        return QCoreApplication::translate("Qnx::Internal::QnxQtVersion", "No SDK path was set up.");
    return QtSupport::BaseQtVersion::invalidReason();
}

QString QnxQtVersion::sdkPath() const
{
    return m_sdkPath;
}

void QnxQtVersion::setSdkPath(const QString &sdkPath)
{
    if (m_sdkPath == sdkPath)
        return;

    m_sdkPath = sdkPath;
    m_environmentUpToDate = false;
}

void QnxQtVersion::updateEnvironment() const
{
    if (!m_environmentUpToDate) {
        m_qnxEnv = environment();
        m_environmentUpToDate = true;
    }
}

QList<Utils::EnvironmentItem> QnxQtVersion::environment() const
{
    return QnxUtils::qnxEnvironment(sdkPath());
}

} // namespace Internal
} // namespace Qnx
