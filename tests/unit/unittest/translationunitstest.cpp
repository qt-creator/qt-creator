/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <diagnosticset.h>
#include <filecontainer.h>
#include <projectpartcontainer.h>
#include <projectpart.h>
#include <projectpartsdonotexistexception.h>
#include <projects.h>
#include <translationunitdoesnotexistexception.h>
#include <translationunitfilenotexitexception.h>
#include <translationunit.h>
#include <translationunitisnullexception.h>
#include <translationunits.h>
#include <unsavedfiles.h>
#include <utf8string.h>


#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;

using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Not;
using testing::Contains;

namespace {

using ::testing::PrintToString;

MATCHER_P3(IsTranslationUnit, filePath, projectPartId, documentRevision,
           std::string(negation ? "isn't" : "is")
           + " translation unit with file path "+ PrintToString(filePath)
           + " and project " + PrintToString(projectPartId)
           + " and document revision " + PrintToString(documentRevision)
           )
{
    return arg.filePath() == filePath
        && arg.projectPartId() == projectPartId
        && arg.documentRevision() == documentRevision;
}

class TranslationUnits : public ::testing::Test
{
protected:
    void SetUp() override;

    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    const Utf8String filePath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    const Utf8String headerPath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.h");
    const Utf8String nonExistingFilePath = Utf8StringLiteral("foo.cpp");
    const Utf8String projectPartId = Utf8StringLiteral("projectPartId");
    const Utf8String nonExistingProjectPartId = Utf8StringLiteral("nonExistingProjectPartId");
};

void TranslationUnits::SetUp()
{
    projects.createOrUpdate({ClangBackEnd::ProjectPartContainer(projectPartId)});
}


TEST_F(TranslationUnits, ThrowForGettingWithWrongFilePath)
{
    ASSERT_THROW(translationUnits.translationUnit(nonExistingFilePath, projectPartId),
                 ClangBackEnd::TranslationUnitDoesNotExistException);

}

TEST_F(TranslationUnits, ThrowForGettingWithWrongProjectPartFilePath)
{
    ASSERT_THROW(translationUnits.translationUnit(filePath, nonExistingProjectPartId),
                 ClangBackEnd::ProjectPartDoNotExistException);

}

TEST_F(TranslationUnits, ThrowForAddingNonExistingFile)
{
    ClangBackEnd::FileContainer fileContainer(nonExistingFilePath, projectPartId);

    ASSERT_THROW(translationUnits.createOrUpdate({fileContainer}),
                 ClangBackEnd::TranslationUnitFileNotExitsException);
}

TEST_F(TranslationUnits, DoNotThrowForAddingNonExistingFileWithUnsavedContent)
{
    ClangBackEnd::FileContainer fileContainer(nonExistingFilePath, projectPartId, Utf8String(), true);

    ASSERT_NO_THROW(translationUnits.createOrUpdate({fileContainer}));
}

TEST_F(TranslationUnits, Add)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, 74u);

    translationUnits.createOrUpdate({fileContainer});

    ASSERT_THAT(translationUnits.translationUnit(filePath, projectPartId),
                IsTranslationUnit(filePath, projectPartId, 74u));
}


TEST_F(TranslationUnits, UpdateUnsavedFileAndCheckForReparse)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, 74u);
    ClangBackEnd::FileContainer headerContainer(headerPath, projectPartId, 74u);
    ClangBackEnd::FileContainer headerContainerWithUnsavedContent(headerPath, projectPartId, Utf8String(), true, 75u);
    translationUnits.createOrUpdate({fileContainer, headerContainer});
    translationUnits.translationUnit(filePath, projectPartId).cxTranslationUnit();

    translationUnits.createOrUpdate({headerContainerWithUnsavedContent});

    ASSERT_TRUE(translationUnits.translationUnit(filePath, projectPartId).isNeedingReparse());
}

TEST_F(TranslationUnits, UpdateUnsavedFileAndCheckForDiagnostics)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, 74u);
    ClangBackEnd::FileContainer headerContainer(headerPath, projectPartId, 74u);
    ClangBackEnd::FileContainer headerContainerWithUnsavedContent(headerPath, projectPartId, Utf8String(), true, 75u);
    translationUnits.createOrUpdate({fileContainer, headerContainer});
    translationUnits.translationUnit(filePath, projectPartId).diagnostics();

    translationUnits.createOrUpdate({headerContainerWithUnsavedContent});

    ASSERT_TRUE(translationUnits.translationUnit(filePath, projectPartId).hasNewDiagnostics());
}

TEST_F(TranslationUnits, RemoveFileAndCheckForDiagnostics)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, 74u);
    ClangBackEnd::FileContainer headerContainer(headerPath, projectPartId, 74u);
    ClangBackEnd::FileContainer headerContainerWithUnsavedContent(headerPath, projectPartId, Utf8String(), true, 75u);
    translationUnits.createOrUpdate({fileContainer, headerContainer});
    translationUnits.translationUnit(filePath, projectPartId).diagnostics();

    translationUnits.remove({headerContainerWithUnsavedContent});

    ASSERT_TRUE(translationUnits.translationUnit(filePath, projectPartId).hasNewDiagnostics());
}

TEST_F(TranslationUnits, DontGetNewerFileContainerIfRevisionIsTheSame)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, 74u);
    translationUnits.createOrUpdate({fileContainer});

    auto newerFileContainers = translationUnits.newerFileContainers({fileContainer});

    ASSERT_THAT(newerFileContainers.size(), 0);
}

TEST_F(TranslationUnits, GetNewerFileContainerIfRevisionIsDifferent)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, 74u);
    ClangBackEnd::FileContainer newerContainer(filePath, projectPartId, 75u);
    translationUnits.createOrUpdate({fileContainer});

    auto newerFileContainers = translationUnits.newerFileContainers({newerContainer});

    ASSERT_THAT(newerFileContainers.size(), 1);
}

TEST_F(TranslationUnits, ThrowForRemovingWithWrongFilePath)
{
    ClangBackEnd::FileContainer fileContainer(nonExistingFilePath, projectPartId);

    ASSERT_THROW(translationUnits.remove({fileContainer}),
                 ClangBackEnd::TranslationUnitDoesNotExistException);
}

TEST_F(TranslationUnits, ThrowForRemovingWithWrongProjectPartFilePath)
{
    ClangBackEnd::FileContainer fileContainer(filePath, nonExistingProjectPartId);

    ASSERT_THROW(translationUnits.remove({fileContainer}),
                 ClangBackEnd::ProjectPartDoNotExistException);
}

TEST_F(TranslationUnits, Remove)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId);
    translationUnits.createOrUpdate({fileContainer});

    translationUnits.remove({fileContainer});

    ASSERT_THROW(translationUnits.translationUnit(filePath, projectPartId),
                 ClangBackEnd::TranslationUnitDoesNotExistException);
}

TEST_F(TranslationUnits, RemoveAllValidIfExceptionIsThrown)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId);
    translationUnits.createOrUpdate({fileContainer});

    ASSERT_THROW(translationUnits.remove({ClangBackEnd::FileContainer(Utf8StringLiteral("dontextist.pro"), projectPartId), fileContainer}),
                 ClangBackEnd::TranslationUnitDoesNotExistException);

    ASSERT_THAT(translationUnits.translationUnits(),
                Not(Contains(TranslationUnit(filePath,
                                             projects.project(projectPartId),
                                             translationUnits))));
}

}


