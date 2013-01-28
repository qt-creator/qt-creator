/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "maemopackageinstaller.h"

#include "maemoglobal.h"

namespace Madde {
namespace Internal {

MaemoDebianPackageInstaller::MaemoDebianPackageInstaller(QObject *parent)
    : AbstractRemoteLinuxPackageInstaller(parent)
{
    connect(this, SIGNAL(stderrData(QString)), SLOT(handleInstallerErrorOutput(QString)));
}

void MaemoDebianPackageInstaller::prepareInstallation()
{
    m_installerStderr.clear();
}

QString MaemoDebianPackageInstaller::installCommandLine(const QString &packageFilePath) const
{
    return MaemoGlobal::devrootshPath() + QLatin1String(" dpkg -i --no-force-downgrade ")
        + packageFilePath;
}

QString MaemoDebianPackageInstaller::cancelInstallationCommandLine() const
{
    return QLatin1String("pkill dpkg");
}

void MaemoDebianPackageInstaller::handleInstallerErrorOutput(const QString &output)
{
    m_installerStderr += output;
}

QString MaemoDebianPackageInstaller::errorString() const
{
    if (m_installerStderr.contains(QLatin1String("Will not downgrade"))) {
        return tr("Installation failed: "
            "You tried to downgrade a package, which is not allowed.");
    } else {
        return QString();
    }
}


HarmattanPackageInstaller::HarmattanPackageInstaller(QObject *parent)
    : AbstractRemoteLinuxPackageInstaller(parent)
{
}

QString HarmattanPackageInstaller::installCommandLine(const QString &packageFilePath) const
{
    return QLatin1String("pkgmgr install-file -f ") + packageFilePath
        + QLatin1String(" |grep -v '%'"); // Otherwise, the terminal codes will screw with our output.
}

QString HarmattanPackageInstaller::cancelInstallationCommandLine() const
{
    return QLatin1String("pkill pkgmgr");
}

} // namespace Internal
} // namespace Madde
