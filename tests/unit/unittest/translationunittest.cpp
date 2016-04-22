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

#include <commandlinearguments.h>
#include <diagnosticset.h>
#include <highlightinginformations.h>
#include <filecontainer.h>
#include <projectpart.h>
#include <projectpartcontainer.h>
#include <projects.h>
#include <translationunitdoesnotexistexception.h>
#include <translationunitfilenotexitexception.h>
#include <clangtranslationunit.h>
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
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::TranslationUnits;

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

    ASSERT_THROW(translationUnit.index(), ClangBackEnd::TranslationUnitIsNullException);
}

TEST_F(TranslationUnit, IndexGetterIsNonNullForValidUnit)
{
    ASSERT_THAT(translationUnit.index(), NotNull());
}

TEST_F(TranslationUnit, ThrowExceptionForGettingCxTranslationUnitForInvalidUnit)
{
    ::TranslationUnit translationUnit;

    ASSERT_THROW(translationUnit.cxTranslationUnit(), ClangBackEnd::TranslationUnitIsNullException);
}

TEST_F(TranslationUnit, CxTranslationUnitGetterIsNonNullForValidUnit)
{
    ASSERT_THAT(translationUnit.cxTranslationUnit(), NotNull());
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
    const auto arguments = translationUnit.commandLineArguments();

    ASSERT_THAT(arguments.at(arguments.count() - 1), Eq(translationUnitFilePath));
}

TEST_F(TranslationUnit, TimeStampForProjectPartChangeIsUpdatedAsNewCxTranslationUnitIsGenerated)
{
    auto lastChangeTimePoint = translationUnit.lastProjectPartChangeTimePoint();
    std::this_thread::sleep_for(std::chrono::steady_clock::duration(1));

    translationUnit.cxTranslationUnit();

    ASSERT_THAT(translationUnit.lastProjectPartChangeTimePoint(), Gt(lastChangeTimePoint));
}

TEST_F(TranslationUnit, TimeStampForProjectPartChangeIsUpdatedAsProjectPartIsCleared)
{
    ProjectPart projectPart = translationUnit.projectPart();
    translationUnit.cxTranslationUnit();
    auto lastChangeTimePoint = translationUnit.lastProjectPartChangeTimePoint();
    std::this_thread::sleep_for(std::chrono::steady_clock::duration(1));

    projectPart.clear();
    translationUnit.cxTranslationUnit();

    ASSERT_THAT(translationUnit.lastProjectPartChangeTimePoint(), Gt(lastChangeTimePoint));
}

TEST_F(TranslationUnit, DocumentRevisionInFileContainerGetter)
{
    translationUnit.setDocumentRevision(74);

    ASSERT_THAT(translationUnit.fileContainer().documentRevision(), 74);
}

TEST_F(TranslationUnit, DependedFilePaths)
{
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
    translationUnit.cxTranslationUnit();

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsReparseAfterChangeOfMainFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NoNeedForReparsingForIndependendFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/otherfiles.h"));

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsReparsingForDependendFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsNoReparsingAfterReparsing)
{
    translationUnit.cxTranslationUnit();
    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    translationUnit.cxTranslationUnit();

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, IsIntactAfterCreation)
{
    translationUnit.cxTranslationUnit();

    ASSERT_TRUE(translationUnit.isIntact());
}

TEST_F(TranslationUnit, IsNotIntactForDeletedFile)
{
    auto translationUnit = createTranslationUnitAndDeleteFile();

    ASSERT_FALSE(translationUnit.isIntact());
}

TEST_F(TranslationUnit, HasNewDiagnosticsAfterCreation)
{
    translationUnit.cxTranslationUnit();

    ASSERT_TRUE(translationUnit.hasNewDiagnostics());
}

TEST_F(TranslationUnit, HasNewDiagnosticsAfterChangeOfMainFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    ASSERT_TRUE(translationUnit.hasNewDiagnostics());
}

TEST_F(TranslationUnit, HasNoNewDiagnosticsForIndependendFile)
{
    translationUnit.cxTranslationUnit();
    translationUnit.diagnostics(); // Reset hasNewDiagnostics

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/otherfiles.h"));

    ASSERT_FALSE(translationUnit.hasNewDiagnostics());
}

TEST_F(TranslationUnit, HasNewDiagnosticsForDependendFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    ASSERT_TRUE(translationUnit.hasNewDiagnostics());
}

TEST_F(TranslationUnit, HasNoNewDiagnosticsAfterGettingDiagnostics)
{
    translationUnit.cxTranslationUnit();
    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    translationUnit.diagnostics(); // Reset hasNewDiagnostics

    ASSERT_FALSE(translationUnit.hasNewDiagnostics());
}

TEST_F(TranslationUnit, HasNewHighlightingInformationsAfterCreation)
{
    translationUnit.cxTranslationUnit();

    ASSERT_TRUE(translationUnit.hasNewHighlightingInformations());
}

TEST_F(TranslationUnit, HasNewHighlightingInformationsForMainFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    ASSERT_TRUE(translationUnit.hasNewHighlightingInformations());
}

TEST_F(TranslationUnit, HasNoNewHighlightingInformationsForIndependendFile)
{
    translationUnit.cxTranslationUnit();
    translationUnit.highlightingInformations();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/otherfiles.h"));

    ASSERT_FALSE(translationUnit.hasNewHighlightingInformations());
}

TEST_F(TranslationUnit, HasNewHighlightingInformationsForDependendFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    ASSERT_TRUE(translationUnit.hasNewHighlightingInformations());
}

TEST_F(TranslationUnit, HasNoNewHighlightingInformationsAfterGettingHighlightingInformations)
{
    translationUnit.cxTranslationUnit();
    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    translationUnit.highlightingInformations();

    ASSERT_FALSE(translationUnit.hasNewHighlightingInformations());
}

TEST_F(TranslationUnit, SetDirtyIfProjectPartIsOutdated)
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});
    translationUnit.cxTranslationUnit();
    projects.createOrUpdate({ProjectPartContainer(projectPartId, {Utf8StringLiteral("-DNEW")})});

    translationUnit.setDirtyIfProjectPartIsOutdated();

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, SetNotDirtyIfProjectPartIsNotOutdated)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfProjectPartIsOutdated();

    ASSERT_FALSE(translationUnit.isNeedingReparse());
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

