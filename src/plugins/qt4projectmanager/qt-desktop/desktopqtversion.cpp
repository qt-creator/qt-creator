/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "desktopqtversion.h"
#include "qt4projectmanagerconstants.h"

#include <qtsupport/qtsupportconstants.h>
#include <proparser/profileevaluator.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfoList>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

DesktopQtVersion::DesktopQtVersion()
    : BaseQtVersion()
{

}

DesktopQtVersion::DesktopQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource)
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
    return QLatin1String(QtSupport::Constants::DESKTOPQT);
}

QStringList DesktopQtVersion::warningReason() const
{
    QStringList ret = BaseQtVersion::warningReason();
    if (qtVersion() >= QtSupport::QtVersionNumber(4, 7, 0) && qmlviewerCommand().isEmpty())
        ret << QCoreApplication::translate("QtVersion", "No qmlviewer installed.");
    return ret;
}

QList<ProjectExplorer::Abi> DesktopQtVersion::detectQtAbis() const
{
    return qtAbisFromLibrary(qtCorePath(versionInfo(), qtVersionString()));
}

QString DesktopQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Desktop", "Qt Version is meant for the desktop");
}

Core::FeatureSet DesktopQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QtSupport::BaseQtVersion::availableFeatures();
    features |= Core::FeatureSet(QtSupport::Constants::FEATURE_DESKTOP);
    features |= Core::Feature(QtSupport::Constants::FEATURE_QMLPROJECT);
    return features;
}

QString DesktopQtVersion::platformName() const
{
    return QLatin1String(QtSupport::Constants::DESKTOP_PLATFORM);
}

QString DesktopQtVersion::platformDisplayName() const
{
    return QLatin1String(QtSupport::Constants::DESKTOP_PLATFORM_TR);
}
