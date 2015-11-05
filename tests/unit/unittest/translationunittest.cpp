/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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
#include <projectpart.h>
#include <projects.h>
#include <translationunitdoesnotexistexception.h>
#include <translationunitfilenotexitexception.h>
#include <translationunit.h>
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


using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::TranslationUnits;

using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;

namespace {

class TranslationUnit : public ::testing::Test
{
protected:
    ::TranslationUnit createTemporaryTranslationUnit();
    QByteArray readContentFromTranslationUnitFile() const;

protected:
    ClangBackEnd::ProjectParts projects;
    ProjectPart projectPart{Utf8StringLiteral("/path/to/projectfile")};
    Utf8String translationUnitFilePath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    ::TranslationUnit translationUnit{translationUnitFilePath,
                                      projectPart,
                                      Utf8StringVector(),
                                      translationUnits};
};

TEST_F(TranslationUnit, DefaultTranslationUnitIsInvalid)
{
    ::TranslationUnit translationUnit;

    ASSERT_TRUE(translationUnit.isNull());
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

TEST_F(TranslationUnit, NeedsReparsingForMainFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    ASSERT_TRUE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, NeedsNoReparsingAfterReparsing)
{
    translationUnit.cxTranslationUnit();
    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    translationUnit.cxTranslationUnit();

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

TEST_F(TranslationUnit, HasNoNewDiagnosticsForIndependendFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/otherfiles.h"));

    ASSERT_FALSE(translationUnit.hasNewDiagnostics());
}

TEST_F(TranslationUnit, HasNewDiagnosticsForDependendFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    ASSERT_TRUE(translationUnit.hasNewDiagnostics());
}

TEST_F(TranslationUnit, HasNewDiagnosticsForMainFile)
{
    translationUnit.cxTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    ASSERT_TRUE(translationUnit.hasNewDiagnostics());
}

TEST_F(TranslationUnit, HasNoNewDiagnosticsAfterGettingDiagnostics)
{
    translationUnit.cxTranslationUnit();
    translationUnit.setDirtyIfDependencyIsMet(translationUnitFilePath);

    translationUnit.diagnostics();

    ASSERT_FALSE(translationUnit.hasNewDiagnostics());
}

TEST_F(TranslationUnit, DeletedFileShouldBeNotSetDirty)
{
    auto translationUnit = createTemporaryTranslationUnit();

    translationUnit.setDirtyIfDependencyIsMet(translationUnit.filePath());

    ASSERT_FALSE(translationUnit.isNeedingReparse());
}

::TranslationUnit TranslationUnit::createTemporaryTranslationUnit()
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

