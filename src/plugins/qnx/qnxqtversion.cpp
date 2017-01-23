/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxqtversion.h"

#include "qnxbaseqtconfigwidget.h"
#include "qnxconstants.h"
#include "qnxutils.h"

#include <coreplugin/featureprovider.h>
#include <proparser/profileevaluator.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QDir>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

static char SDP_PATH_KEY[] = "SDKPath";

QnxQtVersion::QnxQtVersion()
{ }

QnxQtVersion::QnxQtVersion(const Utils::FileName &path, bool isAutoDetected,
                           const QString &autoDetectionSource) :
    QtSupport::BaseQtVersion(path, isAutoDetected, autoDetectionSource)
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
    return QCoreApplication::translate("Qnx::Internal::QnxQtVersion", "QNX %1")
            .arg(QnxUtils::cpuDirShortDescription(cpuDir()));
}

QSet<Core::Id> QnxQtVersion::availableFeatures() const
{
    QSet<Core::Id> features = QtSupport::BaseQtVersion::availableFeatures();
    features.insert(Constants::QNX_QNX_FEATURE);
    features.remove(QtSupport::Constants::FEATURE_QT_CONSOLE);
    features.remove(QtSupport::Constants::FEATURE_QT_WEBKIT);
    return features;
}

QSet<Core::Id> QnxQtVersion::targetDeviceTypes() const
{
    return { Constants::QNX_QNX_OS_TYPE };
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

QString QnxQtVersion::cpuDir() const
{
    ensureMkSpecParsed();
    return m_cpuDir;
}

void QnxQtVersion::parseMkSpec(ProFileEvaluator *evaluator) const
{
    m_cpuDir = evaluator->value(QLatin1String("QNX_CPUDIR"));
    BaseQtVersion::parseMkSpec(evaluator);
}

QVariantMap QnxQtVersion::toMap() const
{
    QVariantMap result = BaseQtVersion::toMap();
    result.insert(QLatin1String(SDP_PATH_KEY), sdpPath());
    return result;
}

void QnxQtVersion::fromMap(const QVariantMap &map)
{
    BaseQtVersion::fromMap(map);
    setSdpPath(QDir::fromNativeSeparators(map.value(QLatin1String(SDP_PATH_KEY)).toString()));
}

QList<ProjectExplorer::Abi> QnxQtVersion::detectQtAbis() const
{
    ensureMkSpecParsed();
    return qtAbisFromLibrary(qtCorePaths());
}

void QnxQtVersion::addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const
{
    QtSupport::BaseQtVersion::addToEnvironment(k, env);
    updateEnvironment();
    env.modify(m_qnxEnv);

    env.prependOrSetLibrarySearchPath(qmakeProperty("QT_INSTALL_LIBS", PropertyVariantDev));
}

Utils::Environment QnxQtVersion::qmakeRunEnvironment() const
{
    if (!sdpPath().isEmpty())
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
    return QtSupport::BaseQtVersion::isValid() && !sdpPath().isEmpty();
}

QString QnxQtVersion::invalidReason() const
{
    if (sdpPath().isEmpty())
        return QCoreApplication::translate("Qnx::Internal::QnxQtVersion",
                                           "No SDP path was set up.");
    return QtSupport::BaseQtVersion::invalidReason();
}

QString QnxQtVersion::sdpPath() const
{
    return m_sdpPath;
}

void QnxQtVersion::setSdpPath(const QString &sdpPath)
{
    if (m_sdpPath == sdpPath)
        return;

    m_sdpPath = sdpPath;
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
    return QnxUtils::qnxEnvironment(sdpPath());
}

} // namespace Internal
} // namespace Qnx
