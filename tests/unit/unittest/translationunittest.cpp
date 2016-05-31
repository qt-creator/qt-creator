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

#include <clangfilepath.h>
#include <clangtranslationunitupdater.h>
#include <commandlinearguments.h>
#include <diagnosticset.h>
#include <highlightingmarks.h>
#include <filecontainer.h>
#include <projectpart.h>
#include <projectpartcontainer.h>
#include <projects.h>
#include <translationunitdoesnotexistexception.h>
#include <translationunitfilenotexitexception.h>
#include <clangtranslationunit.h>
#include <clangtranslationunitcore.h>
#include <translationunitisnullexception.h>
#include <translationunitparseerrorexception.h>
#include <translationunits.h>
#include <unsavedfiles.h>
#include <utf8string.h>

#include <clang-c/Index.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

#include <QTemporaryFile>

#include <chrono>
#include <thread>

using ClangBackEnd::FileContainer;
using ClangBackEnd::FilePath;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::TranslationUnits;
using ClangBackEnd::TranslationUnitUpdateResult;

using testing::IsNull;
using testing::NotNull;
using testing::Eq;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;

namespace {

class TranslationUnit : public ::testing::Test
{
protected:
    void SetUp() override;
    ::TranslationUnit createTranslationUnitAndDeleteFile();
    QByteArray readContentFromTranslationUnitFile() const;

protected:
    ClangBackEnd::ProjectParts projects;
    Utf8String projectPartId{Utf8StringLiteral("/path/to/projectfile")};
    ProjectPart projectPart;
    Utf8String translationUnitFilePath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    ::TranslationUnit translationUnit;
};

TEST_F(TranslationUnit, DefaultTranslationUnitIsInvalid)
{
    ::TranslationUnit translationUnit;

    ASSERT_TRUE(translationUnit.isNull());
}

TEST_F(TranslationUnit, DefaultTranslationUnitIsNotIntact)
{
    ::TranslationUnit translationUnit;

    ASSERT_FALSE(translationUnit.isIntact());
}

TEST_F(TranslationUnit, ThrowExceptionForNonExistingFilePath)
{
    ASSERT_THROW(::TranslationUnit(Utf8StringLiteral("file.cpp"), projectPart, Utf8StringVector(), translationUnits),
                 ClangBackEnd::TranslationUnitFileNotExitsException);
}

TEST_F(TranslationUnit, ThrowNoExceptionForNonExistingFilePathIfDoNotCheckIfFileExistsIsSet)
{
    ASSERT_NO_THROW(::TranslationUnit(Utf8StringLiteral("file.cpp"), projectPart, Utf8StringVector(), translationUnits, ::TranslationUnit::DoNotCheckIfFileExists));
}

TEST_F(TranslationUnit, TranslationUnitIsValid)
{
    ASSERT_FALSE(translationUnit.isNull());
}


TEST_F(TranslationUnit, ThrowExceptionForGettingIndexForInvalidUnit)
{
    ::TranslationUnit translationUnit;

    ASSERT_THROW(translationUnit.translationUnitCore().cxIndex(), ClangBackEnd::TranslationUnitIsNullException);
}

TEST_F(TranslationUnit, ThrowExceptionForGettingCxTranslationUnitForInvalidUnit)
{
    ::TranslationUnit translationUnit;

    ASSERT_THROW(translationUnit.translationUnitCore().cxIndex(), ClangBackEnd::TranslationUnitIsNullException);
}

TEST_F(TranslationUnit, CxTranslationUnitGetterIsNonNullForParsedUnit)
{
    translationUnit.parse();

    ASSERT_THAT(translationUnit.translationUnitCore().cxIndex(), NotNull());
}

TEST_F(TranslationUnit, ThrowExceptionIfGettingFilePathForNullUnit)
{
   ::TranslationUnit translationUnit;

    ASSERT_THROW(translationUnit.filePath(), ClangBackEnd::TranslationUnitIsNullException);
}

TEST_F(TranslationUnit, ResetedTranslationUnitIsNull)
{
    translationUnit.reset();

    ASSERT_TRUE(translationUnit.isNull());
}

TEST_F(TranslationUnit, LastCommandLineArgumentIsFilePath)
{
    const Utf8String nativeFilePath = FilePath::toNativeSeparators(translationUnitFilePath);
    const auto arguments = translationUnit.createUpdater().commandLineArguments();

    ASSERT_THAT(arguments.at(arguments.count() - 1), Eq(nativeFilePath));
}

TEST_F(TranslationUnit, TimeStampForProjectPartChangeIsUpdatedAsNewCxTranslationUnitIsGenerated)
{
    auto lastChangeTimePoint = translationUnit.lastProjectPartChangeTimePoint();
    std::this_thread::sleep_for(std::chrono::steady_clock::duration(1));

    translationUnit.parse();

    ASSERT_THAT(translationUnit.lastProjectPartChangeTimePoint(), Gt(lastChangeTimePoint));
}

TEST_F(TranslationUnit, TimeStampForProjectPartChangeIsUpdatedAsProjectPartIsCleared)
{
    ProjectPart projectPart = translationUnit.projectPart();
    translationUnit.parse();
    auto lastChangeTimePoint = translationUnit.lastProjectPartChangeTimePoint();
    std::this_thread::sleep_for(std::chrono::steady_clock::duration(1));

    projectPart.clear();
    translationUnit.parse();

    ASSERT_THAT(translationUnit.lastProjectPartChangeTimePoint(), Gt(lastChangeTimePoint));
}

TEST_F(TranslationUnit, DocumentRevisionInFileContainerGetter)
{
    translationUnit.setDocumentRevision(74);

    ASSERT_THAT(translationUnit.fileContainer().documentRevision(), 74);
}

TEST_F(TranslationUnit, DependedFilePaths)
{
    translationUnit.parse();

    ASSERT_THAT(translationUnit.dependedFilePaths(),
                AllOf(Contains(translationUnitFilePath),
                      Contains(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"))));
}

TEST_F(TranslationUnit, DeletedFileShouldNotNeedReparsing)
{
    auto translationUnit = createTranslationUnitAndDeleteFile();

    translationUnit.setDirtyIfDependencyIsMet(translationUnit.filePath());

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsNoReparseAfterCreation)
{
    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsReparseAfterChangeOfMainFile)
{
    translationUnit.parse();

    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NoNeedForReparsingForIndependendFile)
{
    translationUnit.parse();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/otherfiles.h"));

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsReparsingForDependendFile)
{
    translationUnit.parse();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsNoReparsingAfterReparsing)
{
    translationUnit.parse();
    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    translationUnit.reparse();

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, IsIntactAfterParsing)
{
    translationUnit.parse();

    ASSERT_TRUE(translationUnit.isIntact());
}

TEST_F(TranslationUnit, IsNotIntactForDeletedFile)
{
    auto translationUnit = createTranslationUnitAndDeleteFile();

    ASSERT_FALSE(translationUnit.isIntact());
}

TEST_F(TranslationUnit, DoesNotNeedReparseAfterParse)
{
    translationUnit.parse();

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsReparseAfterMainFileChanged)
{
    translationUnit.parse();

    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsReparseAfterIncludedFileChanged)
{
    translationUnit.parse();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, DoesNotNeedReparseAfterNotIncludedFileChanged)
{
    translationUnit.parse();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/otherfiles.h"));

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, DoesNotNeedReparseAfterReparse)
{
    translationUnit.parse();
    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    translationUnit.reparse();

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, SetDirtyIfProjectPartIsOutdated)
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});
    translationUnit.parse();
    projects.createOrUpdate({ProjectPartContainer(projectPartId, {Utf8StringLiteral("-DNEW")})});

    translationUnit.setDirtyIfProjectPartIsOutdated();

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, SetNotDirtyIfProjectPartIsNotOutdated)
{
    translationUnit.parse();

    translationUnit.setDirtyIfProjectPartIsOutdated();

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, IncorporateUpdaterResultResetsDirtyness)
{
    translationUnit.setDirtyIfDependencyIsMet(translationUnit.filePath());
    TranslationUnitUpdateResult result;
    result.reparsed = true;
    result.needsToBeReparsedChangeTimePoint = translationUnit.isNeededReparseChangeTimePoint();

    translationUnit.incorporateUpdaterResult(result);

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, IncorporateUpdaterResultDoesNotResetDirtynessIfItWasChanged)
{
    TranslationUnitUpdateResult result;
    result.reparsed = true;
    result.needsToBeReparsedChangeTimePoint = std::chrono::steady_clock::now();
    translationUnit.setDirtyIfDependencyIsMet(translationUnit.filePath());

    translationUnit.incorporateUpdaterResult(result);

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

void TranslationUnit::SetUp()
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});
    projectPart = *projects.findProjectPart(projectPartId);

    const QVector<FileContainer> fileContainer{FileContainer(translationUnitFilePath, projectPartId)};
    const auto createdTranslationUnits = translationUnits.create(fileContainer);
    translationUnit = createdTranslationUnits.front();
}

::TranslationUnit TranslationUnit::createTranslationUnitAndDeleteFile()
{
    QTemporaryFile temporaryFile;
    EXPECT_TRUE(temporaryFile.open());
    EXPECT_TRUE(temporaryFile.write(readContentFromTranslationUnitFile()));
    ::TranslationUnit translationUnit(temporaryFile.fileName(),
                                      projectPart,
                                      Utf8StringVector(),
                                      translationUnits);

    return translationUnit;
}

QByteArray TranslationUnit::readContentFromTranslationUnitFile() const
{
    QFile contentFile(translationUnitFilePath);
    EXPECT_TRUE(contentFile.open(QIODevice::ReadOnly));

    return contentFile.readAll();
}

}

