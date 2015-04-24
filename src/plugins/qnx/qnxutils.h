/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved
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

#ifndef QNX_INTERNAL_QNXUTILS_H
#define QNX_INTERNAL_QNXUTILS_H

#include "qnxconstants.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QTextStream>
#include <QString>

namespace Qnx {
namespace Internal {

class QnxQtVersion;

class ConfigInstallInformation
{
public:
    QString path;
    QString name;
    QString host;
    QString target;
    QString version;
    QString installationXmlFilePath;

    bool isValid() { return !path.isEmpty() && !name.isEmpty() && !host.isEmpty()
                && !target.isEmpty() && !version.isEmpty() && !installationXmlFilePath.isEmpty(); }
};

class QnxUtils
{
public:
    static QString addQuotes(const QString &string);
    static Qnx::QnxArchitecture cpudirToArch(const QString &cpuDir);
    static QStringList searchPaths(Qnx::Internal::QnxQtVersion *qtVersion);
    static QList<Utils::EnvironmentItem> qnxEnvironmentFromEnvFile(const QString &fileName);
    static bool isValidNdkPath(const QString & ndkPath);
    static QString envFilePath(const QString & ndkPath, const QString& targetVersion = QString());
    static QString bbDataDirPath();
    static QString bbqConfigPath();
    static QString defaultTargetVersion(const QString& ndkPath);
    static QList<ConfigInstallInformation> installedConfigs(const QString &configPath = QString());
    static QString sdkInstallerPath(const QString& ndkPath);
    static QString qdeInstallProcess(const QString& ndkPath, const QString &target,
                                     const QString &option, const QString &version = QString());
    static QList<Utils::EnvironmentItem> qnxEnvironment(const QString &ndk);
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_QNXUTILS_H
