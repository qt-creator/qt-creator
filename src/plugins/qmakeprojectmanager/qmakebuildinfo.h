/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmakebuildconfiguration.h"
#include "qmakestep.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kitmanager.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

namespace QmakeProjectManager {

class QmakeBuildInfo : public ProjectExplorer::BuildInfo
{
public:
    QmakeBuildInfo(const QmakeBuildConfigurationFactory *f) : ProjectExplorer::BuildInfo(f) { }

    QString additionalArguments;
    QString makefile;
    QMakeStepConfig config;

    bool operator==(const BuildInfo &o) const final
    {
        if (!ProjectExplorer::BuildInfo::operator==(o))
            return false;

        auto other = static_cast<const QmakeBuildInfo *>(&o);
        return additionalArguments == other->additionalArguments
                && makefile == other->makefile
                && config == other->config;
    }

    QList<ProjectExplorer::Task> reportIssues(const QString &projectPath,
                                              const QString &buildDir) const
    {
        ProjectExplorer::Kit *k = ProjectExplorer::KitManager::kit(kitId);
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
        return version ? version->reportIssues(projectPath, buildDir)
                       : QList<ProjectExplorer::Task>();
    }
};

} // namespace QmakeProjectManager
