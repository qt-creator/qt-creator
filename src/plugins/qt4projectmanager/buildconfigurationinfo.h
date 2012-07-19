/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef BUILDCONFIGURATIONINFO_H
#define BUILDCONFIGURATIONINFO_H

#include "qt4projectmanager_global.h"

#include <coreplugin/featureprovider.h>
#include <qtsupport/baseqtversion.h>

namespace Qt4ProjectManager {

class QT4PROJECTMANAGER_EXPORT BuildConfigurationInfo
{
public:
    explicit BuildConfigurationInfo()
        : buildConfig(QtSupport::BaseQtVersion::QmakeBuildConfig(0)), importing(false)
    { }

    explicit BuildConfigurationInfo(QtSupport::BaseQtVersion::QmakeBuildConfigs bc,
                                    const QString &aa, const QString &d,
                                    bool importing_ = false,
                                    const QString &makefile_ = QString())
        : buildConfig(bc),
          additionalArguments(aa), directory(d),
          importing(importing_),
          makefile(makefile_)
    { }

    bool operator ==(const BuildConfigurationInfo &other) const
    {
        return buildConfig == other.buildConfig
                && additionalArguments == other.additionalArguments
                && directory == other.directory
                && importing == other.importing
                && makefile == other.makefile;
    }

    QtSupport::BaseQtVersion::QmakeBuildConfigs buildConfig;
    QString additionalArguments;
    QString directory;
    bool importing;
    QString makefile;
};

} // namespace Qt4ProjectManager

#endif // BUILDCONFIGURATIONINFO_H
