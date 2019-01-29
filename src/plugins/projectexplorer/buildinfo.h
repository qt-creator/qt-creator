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

class BuildConfigurationFactory;

class PROJECTEXPLORER_EXPORT BuildInfo final
{
public:
    BuildInfo(const BuildConfigurationFactory *f = nullptr) : m_factory(f) { }

    const BuildConfigurationFactory *factory() const { return m_factory; }

    QString displayName;
    QString typeName;
    Utils::FileName buildDirectory;
    Core::Id kitId;
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;

    QVariant extraInfo;
    const BuildConfigurationFactory *m_factory = nullptr;

    bool operator==(const BuildInfo &o) const
    {
        return m_factory == o.m_factory
                && displayName == o.displayName && typeName == o.typeName
                && buildDirectory == o.buildDirectory && kitId == o.kitId
                && buildType == o.buildType;
    }
};

} // namespace ProjectExplorer
