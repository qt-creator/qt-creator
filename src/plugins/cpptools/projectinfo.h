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

#include "cpptools_global.h"

#include "projectpart.h"

#include <projectexplorer/project.h>
#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/toolchain.h>
#include <utils/fileutils.h>

#include <QHash>
#include <QSet>
#include <QVector>

#include <memory>

namespace CppTools {

class CPPTOOLS_EXPORT ProjectInfo
{
public:
    using Ptr = std::shared_ptr<ProjectInfo>;
    static Ptr create(const ProjectExplorer::ProjectUpdateInfo &updateInfo,
                      const QVector<ProjectPart::Ptr> &projectParts);

    const QVector<ProjectPart::Ptr> projectParts() const;
    const QSet<QString> sourceFiles() const;
    QString projectName() const { return m_projectName; }
    Utils::FilePath projectFilePath() const { return m_projectFilePath; }
    Utils::FilePath projectRoot() const { return m_projectFilePath.parentDir(); }
    Utils::FilePath buildRoot() const { return m_buildRoot; }

    // Comparisons
    bool operator ==(const ProjectInfo &other) const;
    bool operator !=(const ProjectInfo &other) const;
    bool definesChanged(const ProjectInfo &other) const;
    bool configurationChanged(const ProjectInfo &other) const;
    bool configurationOrFilesChanged(const ProjectInfo &other) const;

private:
    ProjectInfo(const ProjectExplorer::ProjectUpdateInfo &updateInfo,
                const QVector<ProjectPart::Ptr> &projectParts);

    const QVector<ProjectPart::Ptr> m_projectParts;
    const QString m_projectName;
    const Utils::FilePath m_projectFilePath;
    const Utils::FilePath m_buildRoot;
    const ProjectExplorer::HeaderPaths m_headerPaths;
    const QSet<QString> m_sourceFiles;
    const ProjectExplorer::Macros m_defines;
};

} // namespace CppTools
