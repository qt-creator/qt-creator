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

#include "winceqtversion.h"
#include "qt4projectmanagerconstants.h"
#include <qtsupport/qtsupportconstants.h>
#include <QtCore/QCoreApplication>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

WinCeQtVersion::WinCeQtVersion()
    : QtSupport::BaseQtVersion()
{

}

WinCeQtVersion::WinCeQtVersion(const QString &path, bool isAutodetected, const QString &autodetectionSource)
    : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource)
{

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

QList<ProjectExplorer::Abi> WinCeQtVersion::qtAbis() const
{
    return QList<ProjectExplorer::Abi>()
            << ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture,
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
    return QCoreApplication::translate("QtVersion", "Qt for WinCE", "Qt Version is meant for WinCE");
}
