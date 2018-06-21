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

#include "cpptools_utils.h"
#include "projectpart.h"

#include <functional>

namespace ProjectExplorer { class Project; }

namespace CppTools {
namespace Internal {

class ProjectPartChooser
{
public:
    using FallBackProjectPart = std::function<ProjectPart::Ptr()>;
    using ProjectPartsForFile = std::function<QList<ProjectPart::Ptr>(const QString &filePath)>;
    using ProjectPartsFromDependenciesForFile
        = std::function<QList<ProjectPart::Ptr>(const QString &filePath)>;

public:
    void setFallbackProjectPart(const FallBackProjectPart &getter);
    void setProjectPartsForFile(const ProjectPartsForFile &getter);
    void setProjectPartsFromDependenciesForFile(const ProjectPartsFromDependenciesForFile &getter);

    ProjectPartInfo choose(const QString &filePath,
            const ProjectPartInfo &currentProjectPartInfo,
            const QString &preferredProjectPartId,
            const ProjectExplorer::Project *activeProject,
            Language languagePreference,
            bool projectsUpdated) const;

private:
    FallBackProjectPart m_fallbackProjectPart;
    ProjectPartsForFile m_projectPartsForFile;
    ProjectPartsFromDependenciesForFile m_projectPartsFromDependenciesForFile;
};

} // namespace Internal
} // namespace CppTools
