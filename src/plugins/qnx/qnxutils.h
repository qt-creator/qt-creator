/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved
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

#pragma once

#include "qnxconstants.h"

#include <projectexplorer/abi.h>
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

class QnxTarget
{
public:
    QnxTarget(const Utils::FileName &path, const ProjectExplorer::Abi &abi) :
        m_path(path), m_abi(abi)
    {
    }
    Utils::FileName m_path;
    ProjectExplorer::Abi m_abi;
};

class QnxUtils
{
public:
    static QString addQuotes(const QString &string);
    static QString cpuDirShortDescription(const QString &cpuDir);
    static QList<Utils::EnvironmentItem> qnxEnvironmentFromEnvFile(const QString &fileName);
    static QString envFilePath(const QString &sdpPath);
    static QString defaultTargetVersion(const QString &sdpPath);
    static QList<ConfigInstallInformation> installedConfigs(const QString &configPath = QString());
    static QList<Utils::EnvironmentItem> qnxEnvironment(const QString &sdpPath);
    static QList<QnxTarget> findTargets(const Utils::FileName &basePath);
    static ProjectExplorer::Abi convertAbi(const ProjectExplorer::Abi &abi);
    static QList<ProjectExplorer::Abi> convertAbis(const QList<ProjectExplorer::Abi> &abis);
};

} // namespace Internal
} // namespace Qnx
