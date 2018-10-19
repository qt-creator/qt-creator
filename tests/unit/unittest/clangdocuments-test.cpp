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

#include <clangexceptions.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <unsavedfiles.h>
#include <utf8string.h>

#include <clang-c/Index.h>

using ClangBackEnd::Document;
using ClangBackEnd::UnsavedFiles;

using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Eq;
using testing::Not;
using testing::Contains;

namespace {

using ::testing::PrintToString;

MATCHER_P2(IsDocument, filePath, documentRevision,
           std::string(negation ? "isn't" : "is")
           + " document with file path "+ PrintToString(filePath)
           + " and document revision " + PrintToString(documentRevision)
           )
{
    return arg.filePath() == filePath
        && arg.documentRevision() == documentRevision;
}

class Documents : public ::testing::Test
{
protected:
    void SetUp() override;

protected:
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{unsavedFiles};
    const Utf8String filePath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    const Utf8String otherFilePath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.h");
    const Utf8String headerPath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.h");
    const Utf8String nonExistingFilePath = Utf8StringLiteral("foo.cpp");
    const ClangBackEnd::FileContainer fileContainer{filePath};
    const ClangBackEnd::FileContainer headerContainer{headerPath};
};

using DocumentsSlowTest = Documents;

TEST_F(Documents, ThrowForGettingWithWrongFilePath)
{
    ASSERT_THROW(documents.document(nonExistingFilePath),
                 ClangBackEnd::DocumentDoesNotExistException);

}

TEST_F(Documents, ThrowForAddingNonExistingFile)
{
    ClangBackEnd::FileContainer fileContainer(nonExistingFilePath);

    ASSERT_THROW(documents.create({fileContainer}),
                 ClangBackEnd::DocumentFileDoesNotExistException);
}

TEST_F(Documents, DoNotThrowForAddingNonExistingFileWithUnsavedContent)
{
    ClangBackEnd::FileContainer fileContainer(nonExistingFilePath, Utf8String(), true);

    ASSERT_NO_THROW(documents.create({fileContainer}));
}

TEST_F(Documents, Add)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);

    documents.create({fileContainer});

    ASSERT_THAT(documents.document(filePath),
                IsDocument(filePath, 74u));
}

TEST_F(Documents, CreateWithUnsavedContentSetsDependenciesDirty)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);
    ClangBackEnd::FileContainer fileContainerWithUnsavedContent(otherFilePath, {}, {}, Utf8String(), true, 2u);
    auto dependentDocument = documents.create({fileContainer}).at(0);
    dependentDocument.setDependedFilePaths(QSet<Utf8String>() << filePath << otherFilePath);

    documents.create({fileContainerWithUnsavedContent});

    ASSERT_TRUE(dependentDocument.isDirty());
}

TEST_F(Documents, AddAndTestCreatedTranslationUnit)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);

    auto createdDocuments = documents.create({fileContainer});

    ASSERT_THAT(createdDocuments.front(), IsDocument(filePath, 74u));
}

TEST_F(Documents, ThrowForCreatingAnExistingDocument)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);
    documents.create({fileContainer});

    ASSERT_THROW(documents.create({fileContainer}), ClangBackEnd::DocumentAlreadyExistsException);
}

TEST_F(Documents, ThrowForUpdatingANonExistingDocument)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);
    ASSERT_THROW(documents.update({fileContainer}),
                 ClangBackEnd::DocumentDoesNotExistException);
}

TEST_F(Documents, UpdateSingle)
{
    ClangBackEnd::FileContainer createFileContainer(filePath, {}, {}, 74u);
    ClangBackEnd::FileContainer updateFileContainer(filePath, {}, {}, 75u);
    documents.create({createFileContainer});

    documents.update({updateFileContainer});

    ASSERT_THAT(documents.document(filePath), IsDocument(filePath, 75u));
}

TEST_F(Documents, UpdateReturnsUpdatedDocument)
{
    ClangBackEnd::FileContainer createFileContainer(filePath, {}, {}, 74u);
    ClangBackEnd::FileContainer updateFileContainer(filePath, {}, {}, 75u);
    documents.create({createFileContainer});

    const std::vector<Document> updatedDocuments = documents.update({updateFileContainer});

    ASSERT_THAT(updatedDocuments.size(), Eq(1u));
    ASSERT_THAT(updatedDocuments.front().documentRevision(), Eq(75u));
}

// TODO: Does this test still makes sense?
TEST_F(Documents, UpdateMultiple)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);
    ClangBackEnd::FileContainer fileContainerWithOtherProject(filePath, {}, {}, 74u);
    ClangBackEnd::FileContainer updatedFileContainer(filePath, {}, {}, 75u);
    documents.create({fileContainer, fileContainerWithOtherProject});

    documents.update({updatedFileContainer});

    ASSERT_THAT(documents.document(filePath), IsDocument(filePath, 75u));
}

TEST_F(DocumentsSlowTest, UpdateUnsavedFileAndCheckForReparse)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);
    ClangBackEnd::FileContainer headerContainer(headerPath, {}, {}, 74u);
    ClangBackEnd::FileContainer headerContainerWithUnsavedContent(headerPath, Utf8String(), true, 75u);
    documents.create({fileContainer, headerContainer});
    Document document = documents.document(filePath);
    document.parse();

    documents.update({headerContainerWithUnsavedContent});

    ASSERT_TRUE(documents.document(filePath).isDirty());
}

TEST_F(DocumentsSlowTest, RemoveFileAndCheckForReparse)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);
    ClangBackEnd::FileContainer headerContainer(headerPath, {}, {}, 74u);
    ClangBackEnd::FileContainer headerContainerWithUnsavedContent(headerPath, Utf8String(), true, 75u);
    documents.create({fileContainer, headerContainer});
    Document document = documents.document(filePath);
    document.parse();

    documents.remove({headerContainerWithUnsavedContent});

    ASSERT_TRUE(documents.document(filePath).isDirty());
}

TEST_F(Documents, DontGetNewerFileContainerIfRevisionIsTheSame)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);
    documents.create({fileContainer});

    auto newerFileContainers = documents.newerFileContainers({fileContainer});

    ASSERT_THAT(newerFileContainers.size(), 0);
}

TEST_F(Documents, GetNewerFileContainerIfRevisionIsDifferent)
{
    ClangBackEnd::FileContainer fileContainer(filePath, {}, {}, 74u);
    ClangBackEnd::FileContainer newerContainer(filePath, {}, {}, 75u);
    documents.create({fileContainer});

    auto newerFileContainers = documents.newerFileContainers({newerContainer});

    ASSERT_THAT(newerFileContainers.size(), 1);
}

TEST_F(Documents, ThrowForRemovingWithWrongFilePath)
{
    ClangBackEnd::FileContainer fileContainer(nonExistingFilePath);

    ASSERT_THROW(documents.remove({fileContainer}),
                 ClangBackEnd::DocumentDoesNotExistException);
}

TEST_F(Documents, Remove)
{
    ClangBackEnd::FileContainer fileContainer(filePath);
    documents.create({fileContainer});

    documents.remove({fileContainer});

    ASSERT_THROW(documents.document(filePath),
                 ClangBackEnd::DocumentDoesNotExistException);
}

TEST_F(Documents, RemoveAllValidIfExceptionIsThrown)
{
    ClangBackEnd::FileContainer fileContainer(filePath);
    documents.create({fileContainer});

    ASSERT_THROW(documents.remove({ClangBackEnd::FileContainer(Utf8StringLiteral("dontextist.pro")), fileContainer}),
                 ClangBackEnd::DocumentDoesNotExistException);

    ASSERT_THAT(documents.documents(),
                Not(Contains(Document(filePath, {}, {}, documents))));
}

TEST_F(Documents, HasDocument)
{
    documents.create({{filePath}});

    ASSERT_TRUE(documents.hasDocument(filePath));
}

TEST_F(Documents, HasNotDocument)
{
    ASSERT_FALSE(documents.hasDocument(filePath));
}

TEST_F(Documents, FilteredPositive)
{
    documents.create({{filePath}});
    const auto isMatchingFilePath = [this](const Document &document) {
        return document.filePath() == filePath;
    };

    const bool hasMatches = !documents.filtered(isMatchingFilePath).empty();

    ASSERT_TRUE(hasMatches);
}

TEST_F(Documents, FilteredNegative)
{
    documents.create({{filePath}});
    const auto isMatchingNothing = [](const Document &) {
        return false;
    };

    const bool hasMatches = !documents.filtered(isMatchingNothing).empty();

    ASSERT_FALSE(hasMatches);
}

TEST_F(Documents, DirtyAndVisibleButNotCurrentDocuments)
{
    documents.create({{filePath}});
    documents.updateDocumentsWithChangedDependency(filePath);
    documents.setVisibleInEditors({filePath});
    documents.setUsedByCurrentEditor(Utf8String());

    const bool hasMatches = !documents.dirtyAndVisibleButNotCurrentDocuments().empty();

    ASSERT_TRUE(hasMatches);
}

TEST_F(Documents, isUsedByCurrentEditor)
{
    documents.create({fileContainer});
    auto document = documents.document(fileContainer);

    documents.setUsedByCurrentEditor(filePath);

    ASSERT_TRUE(document.isUsedByCurrentEditor());
}

TEST_F(Documents, IsNotCurrentEditor)
{
    documents.create({fileContainer});
    auto document = documents.document(fileContainer);

    documents.setUsedByCurrentEditor(headerPath);

    ASSERT_FALSE(document.isUsedByCurrentEditor());
}

TEST_F(Documents, IsNotCurrentEditorAfterBeingCurrent)
{
    documents.create({fileContainer});
    auto document = documents.document(fileContainer);
    documents.setUsedByCurrentEditor(filePath);

    documents.setUsedByCurrentEditor(headerPath);

    ASSERT_FALSE(document.isUsedByCurrentEditor());
}

TEST_F(Documents, IsVisibleEditor)
{
    documents.create({fileContainer});
    auto document = documents.document(fileContainer);

    documents.setVisibleInEditors({filePath});

    ASSERT_TRUE(document.isVisibleInEditor());
}

TEST_F(Documents, IsNotVisibleEditor)
{
    documents.create({fileContainer});
    auto document = documents.document(fileContainer);

    documents.setVisibleInEditors({headerPath});

    ASSERT_FALSE(document.isVisibleInEditor());
}

TEST_F(Documents, IsNotVisibleEditorAfterBeingVisible)
{
    documents.create({fileContainer});
    auto document = documents.document(fileContainer);
    documents.setVisibleInEditors({filePath});

    documents.setVisibleInEditors({headerPath});

    ASSERT_FALSE(document.isVisibleInEditor());
}

// TODO: Remove?
void Documents::SetUp()
{
}

}
