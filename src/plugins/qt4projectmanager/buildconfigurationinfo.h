/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BUILDCONFIGURATIONINFO_H
#define BUILDCONFIGURATIONINFO_H

#include "qtversionmanager.h"

namespace Qt4ProjectManager {
struct QT4PROJECTMANAGER_EXPORT BuildConfigurationInfo {
    explicit BuildConfigurationInfo()
        : version(0), buildConfig(QtVersion::QmakeBuildConfig(0)), importing(false), temporaryQtVersion(false)
    {}

    explicit BuildConfigurationInfo(QtVersion *v, QtVersion::QmakeBuildConfigs bc,
                                    const QString &aa, const QString &d, bool importing_ = false, bool temporaryQtVersion_ = false) :
        version(v), buildConfig(bc), additionalArguments(aa), directory(d), importing(importing_), temporaryQtVersion(temporaryQtVersion_)
    { }

    bool isValid() const
    {
        return version != 0;
    }

    QtVersion *version;
    QtVersion::QmakeBuildConfigs buildConfig;
    QString additionalArguments;
    QString directory;
    bool importing;
    bool temporaryQtVersion;

    static QList<BuildConfigurationInfo> importBuildConfigurations(const QString &proFilePath);
    static BuildConfigurationInfo checkForBuild(const QString &directory, const QString &proFilePath);
    static QList<BuildConfigurationInfo> filterBuildConfigurationInfos(const QList<BuildConfigurationInfo> &infos, const QString &id);
};

} // namespace Qt4ProjectManager

#endif // BUILDCONFIGURATIONINFO_H
