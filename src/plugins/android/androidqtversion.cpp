/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidqtversion.h"
#include "androidconstants.h"
#include "qt4projectmanager/qt4projectmanagerconstants.h"

#include <qtsupport/qtsupportconstants.h>

using namespace Android::Internal;

AndroidQtVersion::AndroidQtVersion()
    : QtSupport::BaseQtVersion()
{
}

AndroidQtVersion::AndroidQtVersion(const Utils::FileName &path, bool isAutodetected, const QString &autodetectionSource)
    : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource)
{
}

AndroidQtVersion *AndroidQtVersion::clone() const
{
    return new AndroidQtVersion(*this);
}

QString AndroidQtVersion::type() const
{
    return QLatin1String(Constants::ANDROIDQT);
}

bool AndroidQtVersion::isValid() const
{
    if (!BaseQtVersion::isValid())
        return false;
    if (qtAbis().isEmpty())
        return false;
    return true;
}

QString AndroidQtVersion::invalidReason() const
{
    QString tmp = BaseQtVersion::invalidReason();
    if (tmp.isEmpty() && qtAbis().isEmpty())
        return tr("Failed to detect the ABIs used by the Qt version.");
    return tmp;
}

QList<ProjectExplorer::Abi> AndroidQtVersion::detectQtAbis() const
{
    return QList<ProjectExplorer::Abi>() << ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture, ProjectExplorer::Abi::LinuxOS,
                                                                 ProjectExplorer::Abi::AndroidLinuxFlavor, ProjectExplorer::Abi::ElfFormat,
                                                                 32);
}

QString AndroidQtVersion::description() const
{
    //: Qt Version is meant for Android
    return tr("Android");
}

Core::FeatureSet AndroidQtVersion::availableFeatures() const
{
    Core::FeatureSet features = QtSupport::BaseQtVersion::availableFeatures();
    features |= Core::FeatureSet(QtSupport::Constants::FEATURE_MOBILE);
    return features;
}

QString AndroidQtVersion::platformName() const
{
    return QLatin1String(QtSupport::Constants::ANDROID_PLATFORM);
}

QString AndroidQtVersion::platformDisplayName() const
{
    return QLatin1String(QtSupport::Constants::ANDROID_PLATFORM_TR);
}
