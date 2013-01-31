/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberryqtversion.h"

#include "qnxutils.h"
#include "qnxconstants.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QTextStream>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryQtVersion::BlackBerryQtVersion()
    : QnxAbstractQtVersion()
{
}

BlackBerryQtVersion::BlackBerryQtVersion(QnxArchitecture arch, const Utils::FileName &path, bool isAutoDetected, const QString &autoDetectionSource, const QString &sdkPath)
    : QnxAbstractQtVersion(arch, path, isAutoDetected, autoDetectionSource)
{
    if (QnxUtils::isValidNdkPath(sdkPath))
        setSdkPath(sdkPath);
    else
        setDefaultSdkPath();
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

QMultiMap<QString, QString> BlackBerryQtVersion::environment() const
{
    QTC_CHECK(!sdkPath().isEmpty());
    if (sdkPath().isEmpty())
        return QMultiMap<QString, QString>();

    return QnxUtils::parseEnvironmentFile(QnxUtils::envFilePath(sdkPath()));
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
