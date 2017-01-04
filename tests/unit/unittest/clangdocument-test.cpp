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

#include "googletest.h"

#include <clangclock.h>
#include <clangfilepath.h>
#include <clangtranslationunitupdater.h>
#include <clangtranslationunits.h>
#include <clangtranslationunit.h>
#include <commandlinearguments.h>
#include <diagnosticset.h>
#include <highlightingmarks.h>
#include <filecontainer.h>
#include <projectpart.h>
#include <projectpartcontainer.h>
#include <projects.h>
#include <clangexceptions.h>
#include <clangdocument.h>
#include <clangtranslationunit.h>
#include <clangdocuments.h>
#include <unsavedfiles.h>
#include <utf8string.h>

#include <clang-c/Index.h>

#include <QTemporaryFile>

#include <thread>

using ClangBackEnd::Clock;
using ClangBackEnd::Duration;
using ClangBackEnd::FileContainer;
using ClangBackEnd::FilePath;
using ClangBackEnd::Document;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::Documents;
using ClangBackEnd::TranslationUnitUpdateResult;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::TranslationUnits;

using testing::IsNull;
using testing::NotNull;
using testing::Eq;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;

namespace {

class Document : public ::testing::Test
{
protected:
    void SetUp() override;
    ::Document createDocumentAndDeleteFile();
    QByteArray readContentFromDocumentFile() const;

protected:
    ClangBackEnd::ProjectParts projects;
    Utf8String projectPartId{Utf8StringLiteral("/path/to/projectfile")};
    ProjectPart projectPart;
    Utf8String documentFilePath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    ::Document document;
};

using DocumentSlowTest = Document;

TEST_F(Document, DefaultDocumentIsInvalid)
{
    ::Document document;

    ASSERT_TRUE(document.isNull());
}

TEST_F(Document, DefaultDocumentIsNotIntact)
{
    ::Document document;

    ASSERT_FALSE(document.isIntact());
}

TEST_F(Document, ThrowExceptionForNonExistingFilePath)
{
    ASSERT_THROW(::Document(Utf8StringLiteral("file.cpp"), projectPart, Utf8StringVector(), documents),
                 ClangBackEnd::DocumentFileDoesNotExistException);
}

TEST_F(Document, ThrowNoExceptionForNonExistingFilePathIfDoNotCheckIfFileExistsIsSet)
{
    ASSERT_NO_THROW(::Document(Utf8StringLiteral("file.cpp"), projectPart, Utf8StringVector(), documents, ::Document::DoNotCheckIfFileExists));
}

TEST_F(Document, DocumentIsValid)
{
    ASSERT_FALSE(document.isNull());
}


TEST_F(Document, ThrowExceptionForGettingIndexForInvalidUnit)
{
    ::Document document;

    ASSERT_THROW(document.translationUnit().cxIndex(), ClangBackEnd::DocumentIsNullException);
}

TEST_F(Document, ThrowExceptionForGettingCxTranslationUnitForInvalidUnit)
{
    ::Document document;

    ASSERT_THROW(document.translationUnit().cxIndex(), ClangBackEnd::DocumentIsNullException);
}

TEST_F(DocumentSlowTest, CxTranslationUnitGetterIsNonNullForParsedUnit)
{
    document.parse();

    ASSERT_THAT(document.translationUnit().cxIndex(), NotNull());
}

TEST_F(Document, ThrowExceptionIfGettingFilePathForNullUnit)
{
   ::Document document;

    ASSERT_THROW(document.filePath(), ClangBackEnd::DocumentIsNullException);
}

TEST_F(Document, ResettedDocumentIsNull)
{
    document.reset();

    ASSERT_TRUE(document.isNull());
}

TEST_F(Document, LastCommandLineArgumentIsFilePath)
{
    const Utf8String nativeFilePath = FilePath::toNativeSeparators(documentFilePath);
    const auto arguments = document.createUpdater().commandLineArguments();

    ASSERT_THAT(arguments.at(arguments.count() - 1), Eq(nativeFilePath));
}

TEST_F(DocumentSlowTest, TimeStampForProjectPartChangeIsUpdatedAsNewCxTranslationUnitIsGenerated)
{
    auto lastChangeTimePoint = document.lastProjectPartChangeTimePoint();
    std::this_thread::sleep_for(Duration(1));

    document.parse();

    ASSERT_THAT(document.lastProjectPartChangeTimePoint(), Gt(lastChangeTimePoint));
}

TEST_F(DocumentSlowTest, TimeStampForProjectPartChangeIsUpdatedAsProjectPartIsCleared)
{
    ProjectPart projectPart = document.projectPart();
    document.parse();
    auto lastChangeTimePoint = document.lastProjectPartChangeTimePoint();
    std::this_thread::sleep_for(Duration(1));

    projectPart.clear();
    document.parse();

    ASSERT_THAT(document.lastProjectPartChangeTimePoint(), Gt(lastChangeTimePoint));
}

TEST_F(Document, DocumentRevisionInFileContainerGetter)
{
    document.setDocumentRevision(74);

    ASSERT_THAT(document.fileContainer().documentRevision(), 74);
}

TEST_F(DocumentSlowTest, DependedFilePaths)
{
    document.parse();

    ASSERT_THAT(document.dependedFilePaths(),
                AllOf(Contains(documentFilePath),
                      Contains(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"))));
}

TEST_F(Document, DeletedFileShouldNotNeedReparsing)
{
    auto document = createDocumentAndDeleteFile();

    document.setDirtyIfDependencyIsMet(document.filePath());

    ASSERT_FALSE(document.isNeedingReparse());
}

TEST_F(Document, NeedsNoReparseAfterCreation)
{
    ASSERT_FALSE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, NeedsReparseAfterChangeOfMainFile)
{
    document.parse();

    document.setDirtyIfDependencyIsMet(documentFilePath);

    ASSERT_TRUE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, NoNeedForReparsingForIndependendFile)
{
    document.parse();

    document.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/otherfiles.h"));

    ASSERT_FALSE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, NeedsReparsingForDependendFile)
{
    document.parse();

    document.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    ASSERT_TRUE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, NeedsNoReparsingAfterReparsing)
{
    document.parse();
    document.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    document.reparse();

    ASSERT_FALSE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, IsIntactAfterParsing)
{
    document.parse();

    ASSERT_TRUE(document.isIntact());
}

TEST_F(Document, IsNotIntactForDeletedFile)
{
    auto document = createDocumentAndDeleteFile();

    ASSERT_FALSE(document.isIntact());
}

TEST_F(DocumentSlowTest, DoesNotNeedReparseAfterParse)
{
    document.parse();

    ASSERT_FALSE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, NeedsReparseAfterMainFileChanged)
{
    document.parse();

    document.setDirtyIfDependencyIsMet(documentFilePath);

    ASSERT_TRUE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, NeedsReparseAfterIncludedFileChanged)
{
    document.parse();

    document.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/translationunits.h"));

    ASSERT_TRUE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, DoesNotNeedReparseAfterNotIncludedFileChanged)
{
    document.parse();

    document.setDirtyIfDependencyIsMet(Utf8StringLiteral(TESTDATA_DIR"/otherfiles.h"));

    ASSERT_FALSE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, DoesNotNeedReparseAfterReparse)
{
    document.parse();
    document.setDirtyIfDependencyIsMet(documentFilePath);

    document.reparse();

    ASSERT_FALSE(document.isNeedingReparse());
}

TEST_F(Document, SetDirtyIfProjectPartIsOutdated)
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});
    document.parse();
    projects.createOrUpdate({ProjectPartContainer(projectPartId, {Utf8StringLiteral("-DNEW")})});

    document.setDirtyIfProjectPartIsOutdated();

    ASSERT_TRUE(document.isNeedingReparse());
}

TEST_F(DocumentSlowTest, SetNotDirtyIfProjectPartIsNotOutdated)
{
    document.parse();

    document.setDirtyIfProjectPartIsOutdated();

    ASSERT_FALSE(document.isNeedingReparse());
}

TEST_F(Document, IncorporateUpdaterResultResetsDirtyness)
{
    document.setDirtyIfDependencyIsMet(document.filePath());
    TranslationUnitUpdateResult result;
    result.reparseTimePoint = Clock::now();
    result.needsToBeReparsedChangeTimePoint = document.isNeededReparseChangeTimePoint();
    result.translationUnitId = document.translationUnit().id();

    document.incorporateUpdaterResult(result);

    ASSERT_FALSE(document.isNeedingReparse());
}

TEST_F(Document, IncorporateUpdaterResultDoesNotResetDirtynessIfItWasChanged)
{
    TranslationUnitUpdateResult result;
    result.reparseTimePoint = Clock::now();
    result.needsToBeReparsedChangeTimePoint = Clock::now();
    result.translationUnitId = document.translationUnit().id();
    document.setDirtyIfDependencyIsMet(document.filePath());

    document.incorporateUpdaterResult(result);

    ASSERT_TRUE(document.isNeedingReparse());
}

TEST_F(Document, IncorporateUpdaterResultUpdatesTranslationUnitsReparseTimePoint)
{
    TranslationUnits &translationUnits = document.translationUnits();
    const TranslationUnit initialTranslationUnit = translationUnits.get();
    translationUnits.updateParseTimePoint(initialTranslationUnit.id(), Clock::now());
    const TranslationUnit alternativeTranslationUnit = translationUnits.createAndAppend();
    translationUnits.updateParseTimePoint(alternativeTranslationUnit.id(), Clock::now());
    TranslationUnitUpdateResult result;
    result.reparseTimePoint = Clock::now();
    result.needsToBeReparsedChangeTimePoint = Clock::now();
    result.translationUnitId = initialTranslationUnit.id();
    document.setDirtyIfDependencyIsMet(document.filePath());
    ASSERT_THAT(translationUnits.get().id(), Eq(alternativeTranslationUnit.id()));

    document.incorporateUpdaterResult(result);

    ASSERT_THAT(translationUnits.get().id(), Eq(initialTranslationUnit.id()));
}

void Document::SetUp()
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});
    projectPart = *projects.findProjectPart(projectPartId);

    const QVector<FileContainer> fileContainer{FileContainer(documentFilePath, projectPartId)};
    const auto createdDocuments = documents.create(fileContainer);
    document = createdDocuments.front();
}

::Document Document::createDocumentAndDeleteFile()
{
    QTemporaryFile temporaryFile;
    EXPECT_TRUE(temporaryFile.open());
    EXPECT_TRUE(temporaryFile.write(readContentFromDocumentFile()));
    ::Document document(temporaryFile.fileName(),
                        projectPart,
                        Utf8StringVector(),
                        documents);

    return document;
}

QByteArray Document::readContentFromDocumentFile() const
{
    QFile contentFile(documentFilePath);
    EXPECT_TRUE(contentFile.open(QIODevice::ReadOnly));

    return contentFile.readAll();
}

}

