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

#include "googletest.h"

#include <projectpartsstorageinterface.h>

class MockProjectPartsStorage : public ClangBackEnd::ProjectPartsStorageInterface
{
public:
    MOCK_CONST_METHOD0(fetchProjectParts, ClangBackEnd::ProjectPartContainers());
    MOCK_CONST_METHOD1(
        fetchProjectParts,
        ClangBackEnd::ProjectPartContainers(const ClangBackEnd::ProjectPartIds &projectPartIds));
    MOCK_CONST_METHOD0(fetchAllProjectPartNamesAndIds, ClangBackEnd::Internal::ProjectPartNameIds());
    MOCK_CONST_METHOD1(fetchProjectPartId,
                       ClangBackEnd::ProjectPartId(Utils::SmallStringView projectPartName));
    MOCK_CONST_METHOD1(fetchProjectPartIdUnguarded,
                       ClangBackEnd::ProjectPartId(Utils::SmallStringView projectPartName));
    MOCK_CONST_METHOD1(fetchProjectPartName,
                       Utils::PathString(ClangBackEnd::ProjectPartId projectPartId));
    MOCK_METHOD8(updateProjectPart,
                 void(ClangBackEnd::ProjectPartId projectPartId,
                      const Utils::SmallStringVector &commandLineArgument,
                      const ClangBackEnd::CompilerMacros &compilerMacros,
                      const ClangBackEnd::IncludeSearchPaths &systemIncludeSearchPaths,
                      const ClangBackEnd::IncludeSearchPaths &projectIncludeSearchPaths,
                      Utils::Language language,
                      Utils::LanguageVersion languageVersion,
                      Utils::LanguageExtension languageExtension));
    MOCK_METHOD1(updateProjectParts, void(const ClangBackEnd::ProjectPartContainers &projectParts));
    MOCK_CONST_METHOD1(
        fetchProjectPartArtefact,
        Utils::optional<ClangBackEnd::ProjectPartArtefact>(ClangBackEnd::FilePathId sourceId));
    MOCK_CONST_METHOD1(fetchProjectPartArtefact,
                       Utils::optional<ClangBackEnd::ProjectPartArtefact>(
                           ClangBackEnd::ProjectPartId projectPartId));
    MOCK_METHOD1(resetIndexingTimeStamps,
                 void(const ClangBackEnd::ProjectPartContainers &projectsParts));
    MOCK_METHOD0(transactionBackend, Sqlite::TransactionInterface &());
};
