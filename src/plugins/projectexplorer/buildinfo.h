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

#include "projectexplorer_export.h"

#include "buildconfiguration.h"
#include "task.h"

#include <coreplugin/id.h>
#include <utils/fileutils.h>

namespace ProjectExplorer {

class IBuildConfigurationFactory;

class PROJECTEXPLORER_EXPORT BuildInfo
{
public:
    BuildInfo(const IBuildConfigurationFactory *f) : m_factory(f) { }
    virtual ~BuildInfo();

    const IBuildConfigurationFactory *factory() const { return m_factory; }

    QString displayName;
    QString typeName;
    Utils::FileName buildDirectory;
    Core::Id kitId;
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;

    virtual bool operator==(const BuildInfo &o) const
    {
        return m_factory == o.m_factory
                && displayName == o.displayName && typeName == o.typeName
                && buildDirectory == o.buildDirectory && kitId == o.kitId
                && buildType == o.buildType;
    }

    virtual QList<Task> reportIssues(const QString &projectPath,
                                     const QString &buildDir) const
    {
        Q_UNUSED(projectPath);
        Q_UNUSED(buildDir);
        return QList<Task>();
    }

private:
    const IBuildConfigurationFactory *m_factory;

    friend class IBuildConfigurationFactory;
};

} // namespace ProjectExplorer
