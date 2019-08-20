/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "projectpartcontainer.h"
#include "projectpartstoragestructs.h"

#include <sqlitetransaction.h>
#include <utils/optional.h>
#include <utils/smallstringview.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ClangBackEnd {

class ProjectPartsStorageInterface
{
public:
    ProjectPartsStorageInterface() = default;
    ProjectPartsStorageInterface(const ProjectPartsStorageInterface &) = delete;
    ProjectPartsStorageInterface &operator=(const ProjectPartsStorageInterface &) = delete;

    virtual ProjectPartContainers fetchProjectParts() const = 0;
    virtual ProjectPartContainers fetchProjectParts(const ProjectPartIds &projectPartIds) const = 0;
    virtual ProjectPartId fetchProjectPartIdUnguarded(Utils::SmallStringView projectPartName) const = 0;
    virtual ProjectPartId fetchProjectPartId(Utils::SmallStringView projectPartName) const = 0;
    virtual Utils::PathString fetchProjectPartName(ProjectPartId projectPartId) const = 0;
    virtual Internal::ProjectPartNameIds fetchAllProjectPartNamesAndIds() const = 0;
    virtual void updateProjectPart(ProjectPartId projectPartId,
                                   const Utils::SmallStringVector &commandLineArguments,
                                   const CompilerMacros &compilerMacros,
                                   const ClangBackEnd::IncludeSearchPaths &systemIncludeSearchPaths,
                                   const ClangBackEnd::IncludeSearchPaths &projectIncludeSearchPaths,
                                   Utils::Language language,
                                   Utils::LanguageVersion languageVersion,
                                   Utils::LanguageExtension languageExtension)
        = 0;

    virtual void updateProjectParts(const ProjectPartContainers &projectParts) = 0;

    virtual Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(FilePathId sourceId) const = 0;
    virtual Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(
        ProjectPartId projectPartId) const = 0;
    virtual void resetIndexingTimeStamps(const ProjectPartContainers &projectsParts) = 0;

    virtual Sqlite::TransactionInterface &transactionBackend() = 0;

protected:
    ~ProjectPartsStorageInterface() = default;
};

} // namespace ClangBackEnd
