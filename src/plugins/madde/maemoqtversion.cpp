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
#include "maemoqtversion.h"

#include "maemoconstants.h"
#include "maemoglobal.h"

#include <projectexplorer/kitinformation.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qtsupport/qtsupportconstants.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QTextStream>

using namespace Qt4ProjectManager;

namespace Madde {
namespace Internal {

MaemoQtVersion::MaemoQtVersion()
    : QtSupport::BaseQtVersion(),
      m_isvalidVersion(false),
      m_initialized(false)
{

}

MaemoQtVersion::MaemoQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource),
      m_deviceType(MaemoGlobal::deviceType(path.toString())),
      m_isvalidVersion(false),
      m_initialized(false)
{
    setDisplayName(defaultDisplayName(qtVersionString(), path, false));
}

MaemoQtVersion::~MaemoQtVersion()
{

}

void MaemoQtVersion::fromMap(const QVariantMap &map)
{
    QtSupport::BaseQtVersion::fromMap(map);
    QString path = qmakeCommand().toString();
    m_deviceType = MaemoGlobal::deviceType(path);
}

QString MaemoQtVersion::type() const
{
    return QLatin1String(QtSupport::Constants::MAEMOQT);
}

bool MaemoQtVersion::isValid() const
{
    if (!BaseQtVersion::isValid())
        return false;
    if (!m_initialized) {
        m_isvalidVersion = MaemoGlobal::isValidMaemoQtVersion(qmakeCommand().toString(), m_deviceType);
        m_initialized = true;
    }
    return m_isvalidVersion;
}

MaemoQtVersion *MaemoQtVersion::clone() const
{
    return new MaemoQtVersion(*this);
}

QList<ProjectExplorer::Abi> MaemoQtVersion::detectQtAbis() const
{
    QList<ProjectExplorer::Abi> result;
    if (!isValid())
        return result;
    if (m_deviceType == Maemo5OsType) {
        result.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::MaemoLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
                                           32));
    } else if (m_deviceType == HarmattanOsType) {
        result.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::HarmattanLinuxFlavor,
                                           ProjectExplorer::Abi::ElfFormat,
                                           32));
    }
    return result;
}

QString MaemoQtVersion::description() const
{
    if (m_deviceType == Maemo5OsType)
        return QCoreApplication::translate("QtVersion", "Maemo", "Qt Version is meant for Maemo5");
    if (m_deviceType == HarmattanOsType)
        return QCoreApplication::translate("QtVersion", "Harmattan ", "Qt Version is meant for Harmattan");
    return QString();
}

bool MaemoQtVersion::supportsShadowBuilds() const
{
    return !Utils::HostOsInfo::isWindowsHost();
}

Core::Id MaemoQtVersion::deviceType() const
{
    return m_deviceType;
}

Core::FeatureSet MaemoQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QtSupport::BaseQtVersion::availableFeatures();
    if (qtVersion() >= QtSupport::QtVersionNumber(4, 7, 4)) //no reliable test for components, yet.
        features |= Core::FeatureSet(QtSupport::Constants::FEATURE_QTQUICK_COMPONENTS_MEEGO);
    features |= Core::FeatureSet(QtSupport::Constants::FEATURE_MOBILE);

    if (deviceType() != Maemo5OsType) //Only Maemo5 has proper support for Widgets
        features.remove(Core::Feature(QtSupport::Constants::FEATURE_QWIDGETS));

    return features;
}

QString MaemoQtVersion::platformName() const
{
    if (m_deviceType == Maemo5OsType)
        return QLatin1String(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM);
    return QLatin1String(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM);
}

QString MaemoQtVersion::platformDisplayName() const
{
    if (m_deviceType == Maemo5OsType)
        return QLatin1String(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM_TR);
    return QLatin1String(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM_TR);
}

void MaemoQtVersion::addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const
{
    Q_UNUSED(k);
    const QString maddeRoot = MaemoGlobal::maddeRoot(qmakeCommand().toString());

    // Needed to make pkg-config stuff work.
    Utils::FileName sysRoot = ProjectExplorer::SysRootKitInformation::sysRoot(k);
    env.prependOrSet(QLatin1String("SYSROOT_DIR"), sysRoot.toUserOutput());
    env.prependOrSetPath(QDir::toNativeSeparators(QString::fromLatin1("%1/madbin")
        .arg(maddeRoot)));
    env.prependOrSetPath(QDir::toNativeSeparators(QString::fromLatin1("%1/madlib")
        .arg(maddeRoot)));
    env.prependOrSet(QLatin1String("PERL5LIB"),
        QDir::toNativeSeparators(QString::fromLatin1("%1/madlib/perl5").arg(maddeRoot)));

    env.prependOrSetPath(QDir::toNativeSeparators(QString::fromLatin1("%1/bin").arg(maddeRoot)));
    env.prependOrSetPath(QDir::toNativeSeparators(QString::fromLatin1("%1/bin")
        .arg(MaemoGlobal::targetRoot(qmakeCommand().toString()))));

    // Actually this is tool chain related, but since we no longer have a tool chain...
    const QString manglePathsKey = QLatin1String("GCCWRAPPER_PATHMANGLE");
    if (!env.hasKey(manglePathsKey)) {
        const QStringList pathsToMangle = QStringList() << QLatin1String("/lib")
            << QLatin1String("/opt") << QLatin1String("/usr");
        env.set(manglePathsKey, pathsToMangle.join(QLatin1String(":")));
    }
}

} // namespace Internal
} // namespace Madde
