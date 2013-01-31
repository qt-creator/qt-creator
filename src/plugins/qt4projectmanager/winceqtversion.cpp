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

#include "winceqtversion.h"
#include "qt4projectmanagerconstants.h"
#include <qtsupport/qtsupportconstants.h>
#include <QCoreApplication>

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
    if (0 == archType.compare(QLatin1String("x86"), Qt::CaseInsensitive))
        m_archType = ProjectExplorer::Abi::X86Architecture;
    else if (0 == archType.compare(QLatin1String("mipsii"), Qt::CaseInsensitive))
        m_archType = ProjectExplorer::Abi::MipsArchitecture;
    setDisplayName(defaultDisplayName(qtVersionString(), path, false));
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
    return QLatin1String(QtSupport::Constants::WINCEQT);
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

    const QStringList splitSpec = mkspec().toString().split(QLatin1Char('-'));
    if (splitSpec.length() == 3) {
        const QString archString = splitSpec.value(1);
        if (archString.contains(QLatin1String("x86"), Qt::CaseInsensitive))
            m_archType = ProjectExplorer::Abi::X86Architecture;
        else if (archString.contains(QLatin1String("mips"), Qt::CaseInsensitive))
            m_archType = ProjectExplorer::Abi::MipsArchitecture;
    }
}

QString WinCeQtVersion::platformName() const
{
    return QLatin1String(QtSupport::Constants::WINDOWS_CE_PLATFORM);
}

QString WinCeQtVersion::platformDisplayName() const
{
    return QLatin1String(QtSupport::Constants::WINDOWS_CE_PLATFORM_TR);
}
