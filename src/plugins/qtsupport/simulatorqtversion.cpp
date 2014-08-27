/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "simulatorqtversion.h"
#include "qtsupportconstants.h"

#include <coreplugin/featureprovider.h>

#include <QCoreApplication>

using namespace QtSupport;
using namespace QtSupport::Internal;

SimulatorQtVersion::SimulatorQtVersion()
    : BaseQtVersion()
{

}

SimulatorQtVersion::SimulatorQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource)
{
    setUnexpandedDisplayName(defaultUnexpandedDisplayName(path, false));
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
    return QLatin1String(Constants::SIMULATORQT);
}

QStringList SimulatorQtVersion::warningReason() const
{
    QStringList ret = BaseQtVersion::warningReason();
    if (qtVersion() >= QtVersionNumber(5, 0, 0) && qmlsceneCommand().isEmpty())
        ret << QCoreApplication::translate("QtVersion", "No qmlscene installed.");
    if (qtVersion() >= QtVersionNumber(4, 7, 0) && qmlviewerCommand().isEmpty())
        ret << QCoreApplication::translate("QtVersion", "No qmlviewer installed.");
    return ret;
}

QList<ProjectExplorer::Abi> SimulatorQtVersion::detectQtAbis() const
{
    ensureMkSpecParsed();
    return qtAbisFromLibrary(qtCorePaths(versionInfo(), qtVersionString()));
}

QString SimulatorQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Qt Simulator", "Qt Version is meant for Qt Simulator");
}

Core::FeatureSet SimulatorQtVersion::availableFeatures() const
{
    Core::FeatureSet features = BaseQtVersion::availableFeatures();
    features |= Core::FeatureSet(Constants::FEATURE_MOBILE);
    return features;
}

bool SimulatorQtVersion::supportsPlatform(const QString &platformName) const
{
    return platformName.isEmpty();
}
