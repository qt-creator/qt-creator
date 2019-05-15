/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "desktopqtversion.h"
#include "qtsupportconstants.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <remotelinux/remotelinux_constants.h>
#include <coreplugin/featureprovider.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFileInfo>

namespace QtSupport {

DesktopQtVersion::DesktopQtVersion()
    : BaseQtVersion()
{

}

QStringList DesktopQtVersion::warningReason() const
{
    QStringList ret = BaseQtVersion::warningReason();
    if (qtVersion() >= QtVersionNumber(5, 0, 0)) {
        if (qmlsceneCommand().isEmpty())
            ret << QCoreApplication::translate("QtVersion", "No qmlscene installed.");
    }
    return ret;
}

QString DesktopQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Desktop", "Qt Version is meant for the desktop");
}

QSet<Core::Id> DesktopQtVersion::availableFeatures() const
{
    QSet<Core::Id> features = BaseQtVersion::availableFeatures();
    features.insert(Constants::FEATURE_DESKTOP);
    features.insert(Constants::FEATURE_QMLPROJECT);
    return features;
}

QSet<Core::Id> DesktopQtVersion::targetDeviceTypes() const
{
    QSet<Core::Id> result = {ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE};
    if (Utils::contains(qtAbis(), [](const ProjectExplorer::Abi a) { return a.os() == ProjectExplorer::Abi::LinuxOS; }))
        result.insert(RemoteLinux::Constants::GenericLinuxOsType);
    return result;
}

void DesktopQtVersion::fromMap(const QVariantMap &map)
{
    BaseQtVersion::fromMap(map);
    // Clear the cached qmlscene command, it might not match the restored path anymore.
    m_qmlsceneCommand.clear();
}

QString DesktopQtVersion::qmlsceneCommand() const
{
    if (!isValid())
        return QString();

    if (!m_qmlsceneCommand.isNull())
        return m_qmlsceneCommand;

    ensureMkSpecParsed();

    const QString path =
        qmlBinPath().pathAppended(Utils::HostOsInfo::withExecutableSuffix("qmlscene")).toString();

    m_qmlsceneCommand = QFileInfo(path).isFile() ? path : QString();

    return m_qmlsceneCommand;
}

namespace Internal {

// Factory

DesktopQtVersionFactory::DesktopQtVersionFactory()
{
    setQtVersionCreator([] { return new DesktopQtVersion; });
    setSupportedType(QtSupport::Constants::DESKTOPQT);
    setPriority(0); // Lowest of all, we want to be the fallback
    // No further restrictions. We are the fallback :) so we don't care what kind of qt it is.
}

} // Internal
} // QtSupport
