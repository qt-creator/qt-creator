/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "maemoqtversion.h"
#include "qt4projectmanagerconstants.h"
#include "qt-maemo/maemoglobal.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QDir>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

MaemoQtVersion::MaemoQtVersion()
    : BaseQtVersion()
{

}

MaemoQtVersion::MaemoQtVersion(const QString &path, bool isAutodetected, const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource),
      m_osVersion(MaemoGlobal::version(path)),
      m_isvalidVersion(MaemoGlobal::isValidMaemoQtVersion(path, m_osVersion))
{

}

MaemoQtVersion::~MaemoQtVersion()
{

}

void MaemoQtVersion::fromMap(const QVariantMap &map)
{
    BaseQtVersion::fromMap(map);
    QString path = qmakeCommand();
    m_osVersion = MaemoGlobal::version(path);
    m_isvalidVersion = MaemoGlobal::isValidMaemoQtVersion(path, m_osVersion);
}

QString MaemoQtVersion::type() const
{
    return Constants::MAEMOQT;
}

MaemoQtVersion *MaemoQtVersion::clone() const
{
    return new MaemoQtVersion(*this);
}

QString MaemoQtVersion::systemRoot() const
{
    if (m_systemRoot.isNull()) {
        QFile file(QDir::cleanPath(MaemoGlobal::targetRoot(qmakeCommand()))
                   + QLatin1String("/information"));
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                const QString &line = stream.readLine().trimmed();
                const QStringList &list = line.split(QLatin1Char(' '));
                if (list.count() <= 1)
                    continue;
                if (list.at(0) == QLatin1String("sysroot")) {
                    m_systemRoot = MaemoGlobal::maddeRoot(qmakeCommand())
                            + QLatin1String("/sysroots/") + list.at(1);
                }
            }
        }
    }
    return m_systemRoot;
}

QList<ProjectExplorer::Abi> MaemoQtVersion::qtAbis() const
{
    QList<ProjectExplorer::Abi> result;
    if (!m_isvalidVersion)
        return result;
    if (m_osVersion == MaemoDeviceConfig::Maemo5) {
        result.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::MaemoLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
                                           32));
    } else if (m_osVersion == MaemoDeviceConfig::Maemo6) {
        result.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::HarmattanLinuxFlavor,
                                           ProjectExplorer::Abi::ElfFormat,
                                           32));
    } else if (m_osVersion == MaemoDeviceConfig::Meego) {
        result.append(ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                           ProjectExplorer::Abi::MeegoLinuxFlavor,
                                           ProjectExplorer::Abi::ElfFormat, 32));
    }
    return result;
}

bool MaemoQtVersion::supportsTargetId(const QString &id) const
{
    return supportedTargetIds().contains(id);
}

QSet<QString> MaemoQtVersion::supportedTargetIds() const
{
    QSet<QString> result;
    if (!m_isvalidVersion)
        return result;
    if (m_osVersion == MaemoDeviceConfig::Maemo5) {
        result.insert(QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID));
    } else if (m_osVersion == MaemoDeviceConfig::Maemo6) {
        result.insert(QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID));
    } else if (m_osVersion == MaemoDeviceConfig::Meego) {
        result.insert(QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID));
    }
    return result;
}

QString MaemoQtVersion::description() const
{
    if (m_osVersion == MaemoDeviceConfig::Maemo5)
        return QCoreApplication::translate("QtVersion", "Maemo", "Qt Version is meant for Maemo5");
    else if (m_osVersion == MaemoDeviceConfig::Maemo6)
        return QCoreApplication::translate("QtVersion", "Harmattan ", "Qt Version is meant for Harmattan");
    else if (m_osVersion == MaemoDeviceConfig::Meego)
        return QCoreApplication::translate("QtVersion", "Meego", "Qt Version is meant for Meego");
    return QString();
}

bool MaemoQtVersion::supportsShadowBuilds() const
{
#ifdef Q_OS_WIN
    return false;
#endif
    return true;
}

MaemoDeviceConfig::OsVersion MaemoQtVersion::osVersion() const
{
    return m_osVersion;
}
