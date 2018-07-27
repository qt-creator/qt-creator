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

#include <coreplugin/id.h>

#include <QObject>

namespace ProjectExplorer { class Target; }

namespace Utils { class FileName; }

namespace Android {
namespace Constants {

const char AndroidPackageSourceDir[] = "AndroidPackageSourceDir"; // QString
const char AndroidDeploySettingsFile[] = "AndroidDeploySettingsFile"; // QString
const char AndroidExtraLibs[] = "AndroidExtraLibs";  // QStringList
const char AndroidArch[] = "AndroidArch"; // QString

} // namespace Constants

class ANDROID_EXPORT AndroidQtSupport : public QObject
{
    Q_OBJECT

protected:
    AndroidQtSupport();
    ~AndroidQtSupport() override;

public:
    enum BuildType {
        DebugBuild,
        ReleaseBuildUnsigned,
        ReleaseBuildSigned
    };

    virtual bool canHandle(const ProjectExplorer::Target *target) const = 0;
    virtual QStringList soLibSearchPath(const ProjectExplorer::Target *target) const = 0;
    virtual QStringList projectTargetApplications(const ProjectExplorer::Target *target) const = 0;

    virtual QVariant targetData(Core::Id role, const ProjectExplorer::Target *target) const = 0;
    virtual bool setTargetData(Core::Id role, const QVariant &value,
                               const ProjectExplorer::Target *target) const = 0;

    virtual bool parseInProgress(const ProjectExplorer::Target *target) const = 0;
    virtual bool validParse(const ProjectExplorer::Target *target) const = 0;
    virtual bool extraLibraryEnabled(const ProjectExplorer::Target *target) const = 0;
    virtual Utils::FileName projectFilePath(const ProjectExplorer::Target *target) const = 0;

    virtual void addFiles(const ProjectExplorer::Target *target, const QString &buildKey,
                          const QStringList &addedFiles) const = 0;
};

} // namespace Android
