/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "android_global.h"

#include <QObject>
#include <QList>

namespace ProjectExplorer {
    class DeployConfiguration;
    class ProcessParameters;
    class Project;
    class Target;
}

namespace Utils { class FileName; }

namespace Android {

class ANDROID_EXPORT AndroidQtSupport : public QObject
{
    Q_OBJECT

public:
    enum BuildType {
        DebugBuild,
        ReleaseBuildUnsigned,
        ReleaseBuildSigned
    };

public:
    virtual bool canHandle(const ProjectExplorer::Target *target) const = 0;
    virtual QStringList soLibSearchPath(const ProjectExplorer::Target *target) const = 0;
    virtual QStringList androidExtraLibs(const ProjectExplorer::Target *target) const = 0;
    virtual QStringList projectTargetApplications(const ProjectExplorer::Target *target) const = 0;
    virtual Utils::FileName apkPath(ProjectExplorer::Target *target) const;
    virtual Utils::FileName androiddeployqtPath(ProjectExplorer::Target *target) const = 0;
    virtual Utils::FileName androiddeployJsonPath(ProjectExplorer::Target *target) const = 0;
    virtual void manifestSaved(const ProjectExplorer::Target *target) = 0;
    virtual Utils::FileName manifestSourcePath(const ProjectExplorer::Target *target) = 0;
};

} // namespace Android
