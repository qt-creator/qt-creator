/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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

#include "blackberryruntimeconfiguration.h"

#include "qnxconstants.h"

#include <QVariantMap>
#include <QFileInfo>

namespace Qnx {
namespace Internal {

const QLatin1String PathKey("Path");
const QLatin1String DisplayNameKey("DisplayName");
const QLatin1String VersionKey("Version");

BlackBerryRuntimeConfiguration::BlackBerryRuntimeConfiguration(
        const QString &path,
        const BlackBerryVersionNumber &version)
    : m_path(path)
{
    if (!version.isEmpty())
        m_version = version;
    else
        m_version = BlackBerryVersionNumber::fromFileName(QFileInfo(path).baseName(),
                                                          QRegExp(QLatin1String("^runtime_(.*)$")));

    m_displayName = QObject::tr("Runtime ") + m_version.toString();
}

BlackBerryRuntimeConfiguration::BlackBerryRuntimeConfiguration(const QVariantMap &data)
{
    m_path = data.value(QLatin1String(PathKey)).toString();
    m_displayName = data.value(QLatin1String(DisplayNameKey)).toString();
    m_version = BlackBerryVersionNumber(data.value(QLatin1String(VersionKey)).toString());
}

QString BlackBerryRuntimeConfiguration::path() const
{
    return m_path;
}

QString BlackBerryRuntimeConfiguration::displayName() const
{
    return m_displayName;
}

BlackBerryVersionNumber BlackBerryRuntimeConfiguration::version() const
{
    return m_version;
}

QVariantMap BlackBerryRuntimeConfiguration::toMap() const
{
    QVariantMap data;
    data.insert(QLatin1String(Qnx::Constants::QNX_BB_KEY_CONFIGURATION_TYPE),
                QLatin1String(Qnx::Constants::QNX_BB_RUNTIME_TYPE));
    data.insert(QLatin1String(PathKey), m_path);
    data.insert(QLatin1String(DisplayNameKey), m_displayName);
    data.insert(QLatin1String(VersionKey), m_version.toString());
    return data;
}

}
}
