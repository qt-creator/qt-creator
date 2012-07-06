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

#include "simulatorqtversion.h"
#include "qt4projectmanagerconstants.h"

#include <qtsupport/qtsupportconstants.h>
#include <proparser/profileevaluator.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfoList>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

SimulatorQtVersion::SimulatorQtVersion()
    : QtSupport::BaseQtVersion()
{

}

SimulatorQtVersion::SimulatorQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource)
{

}

SimulatorQtVersion::~SimulatorQtVersion()
{

}

SimulatorQtVersion *SimulatorQtVersion::clone() const
{
    return new SimulatorQtVersion(*this);
}

QString SimulatorQtVersion::type() const
{
    return QLatin1String(QtSupport::Constants::SIMULATORQT);
}

QStringList SimulatorQtVersion::warningReason() const
{
    QStringList ret;
    if (qtAbis().count() == 1 && qtAbis().first().isNull())
        ret << QCoreApplication::translate("QtVersion", "ABI detection failed: Make sure to use a matching tool chain when building.");
    if (qtVersion() >= QtSupport::QtVersionNumber(4, 7, 0) && qmlviewerCommand().isEmpty())
        ret << QCoreApplication::translate("QtVersion", "No qmlviewer installed.");
    return ret;
}

QList<ProjectExplorer::Abi> SimulatorQtVersion::detectQtAbis() const
{
    ensureMkSpecParsed();
    return qtAbisFromLibrary(qtCorePath(versionInfo(), qtVersionString()));
}

QString SimulatorQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Qt Simulator", "Qt Version is meant for Qt Simulator");
}

Core::FeatureSet SimulatorQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QtSupport::BaseQtVersion::availableFeatures();
    if (qtVersion() >= QtSupport::QtVersionNumber(4, 7, 4)) //no reliable test for components, yet.
           features |= Core::FeatureSet(QtSupport::Constants::FEATURE_QTQUICK_COMPONENTS_MEEGO)
                   | Core::FeatureSet(QtSupport::Constants::FEATURE_QTQUICK_COMPONENTS_SYMBIAN);
    features |= Core::FeatureSet(QtSupport::Constants::FEATURE_MOBILE);

    return features;
}

bool SimulatorQtVersion::supportsPlatform(const QString &platformName) const
{
    return (platformName == QtSupport::Constants::SYMBIAN_PLATFORM
            || platformName == QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM
            || platformName.isEmpty());
}
