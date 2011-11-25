/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "winceqtversion.h"
#include "qt4projectmanagerconstants.h"
#include <qtsupport/qtsupportconstants.h>
#include <QtCore/QCoreApplication>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

WinCeQtVersion::WinCeQtVersion()
    : QtSupport::BaseQtVersion(),
      m_archType(ProjectExplorer::Abi::ArmArchitecture)
{
}

WinCeQtVersion::WinCeQtVersion(const Utils::FileName &path, const QString &archType,
        bool isAutodetected, const QString &autodetectionSource)
  : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource),
    m_archType(ProjectExplorer::Abi::ArmArchitecture)
{
    if (0 == archType.compare("x86", Qt::CaseInsensitive))
        m_archType = ProjectExplorer::Abi::X86Architecture;
    else if (0 == archType.compare("mipsii", Qt::CaseInsensitive))
        m_archType = ProjectExplorer::Abi::MipsArchitecture;
}

WinCeQtVersion::~WinCeQtVersion()
{
}

WinCeQtVersion *WinCeQtVersion::clone() const
{
    return new WinCeQtVersion(*this);
}

QString WinCeQtVersion::type() const
{
    return QtSupport::Constants::WINCEQT;
}

QList<ProjectExplorer::Abi> WinCeQtVersion::detectQtAbis() const
{
    return QList<ProjectExplorer::Abi>()
            << ProjectExplorer::Abi(m_archType,
                                    ProjectExplorer::Abi::WindowsOS,
                                    ProjectExplorer::Abi::WindowsCEFlavor,
                                    ProjectExplorer::Abi::PEFormat,
                                    false);
}

bool WinCeQtVersion::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::DESKTOP_TARGET_ID);
}

QSet<QString> WinCeQtVersion::supportedTargetIds() const
{
    return QSet<QString>() << QLatin1String(Constants::DESKTOP_TARGET_ID);
}

QString WinCeQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion",
        "Qt for WinCE", "Qt Version is meant for WinCE");
}

void WinCeQtVersion::fromMap(const QVariantMap &map)
{
    BaseQtVersion::fromMap(map);

    // Default to an ARM architecture, then use the makespec to see what
    // the architecture is. This assumes that a WinCE makespec will be
    // named <Description>-<Architecture>-<Compiler> with no other '-' characters.
    m_archType = ProjectExplorer::Abi::ArmArchitecture;

    const QStringList splitSpec = mkspec().toString().split("-");
    if (splitSpec.length() == 3) {
        const QString archString = splitSpec.value(1);
        if (archString.contains("x86", Qt::CaseInsensitive))
            m_archType = ProjectExplorer::Abi::X86Architecture;
        else if (archString.contains("mips", Qt::CaseInsensitive))
            m_archType = ProjectExplorer::Abi::MipsArchitecture;
    }
}

QVariantMap WinCeQtVersion::toMap() const
{
    return BaseQtVersion::toMap();
}
