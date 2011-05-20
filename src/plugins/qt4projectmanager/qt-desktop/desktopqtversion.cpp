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

#include "desktopqtversion.h"
#include "qt4projectmanagerconstants.h"

#include <qtsupport/qtsupportconstants.h>
#include <proparser/profileevaluator.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfoList>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

DesktopQtVersion::DesktopQtVersion()
    : BaseQtVersion(),
      m_qtAbisUpToDate(false)
{

}

DesktopQtVersion::DesktopQtVersion(const QString &path, bool isAutodetected, const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource),
      m_qtAbisUpToDate(false)
{

}

DesktopQtVersion::~DesktopQtVersion()
{

}

DesktopQtVersion *DesktopQtVersion::clone() const
{
    return new DesktopQtVersion(*this);
}

QString DesktopQtVersion::type() const
{
    return QtSupport::Constants::DESKTOPQT;
}

bool DesktopQtVersion::isValid() const
{
    if (!BaseQtVersion::isValid())
        return false;
    if (qtAbis().isEmpty())
        return false;
    return true;
}

QString DesktopQtVersion::invalidReason() const
{
    QString tmp = BaseQtVersion::invalidReason();
    if (tmp.isEmpty() && qtAbis().isEmpty())
        return QCoreApplication::translate("QtVersion", "Failed to detect the ABI(s) used by the Qt version.");
    return tmp;
}

QList<ProjectExplorer::Abi> DesktopQtVersion::qtAbis() const
{
    if (!m_qtAbisUpToDate) {
        m_qtAbisUpToDate = true;
        ensureMkSpecParsed();
        m_qtAbis = qtAbisFromLibrary(qtCorePath(versionInfo(), qtVersionString()));
    }
    return m_qtAbis;
}

bool DesktopQtVersion::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::DESKTOP_TARGET_ID);
}

QSet<QString> DesktopQtVersion::supportedTargetIds() const
{
    return QSet<QString>() << QLatin1String(Constants::DESKTOP_TARGET_ID);
}

QString DesktopQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Desktop", "Qt Version is meant for the desktop");
}
