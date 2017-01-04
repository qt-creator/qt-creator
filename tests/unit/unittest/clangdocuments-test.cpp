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

#include <projectpart.h>
#include <clangexceptions.h>
#include <projects.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <unsavedfiles.h>
#include <utf8string.h>

#include <clang-c/Index.h>

using ClangBackEnd::Document;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::ProjectPartContainer;

using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Eq;
using testing::Not;
using testing::Contains;

namespace {

using ::testing::PrintToString;

MATCHER_P3(IsDocument, filePath, projectPartId, documentRevision,
           std::string(negation ? "isn't" : "is")
           + " document with file path "+ PrintToString(filePath)
           + " and project " + PrintToString(projectPartId)
           + " and document revision " + PrintToString(documentRevision)
           )
{
    return arg.filePath() == filePath
        && arg.projectPartId() == projectPartId
        && arg.documentRevision() == documentRevision;
}

class Documents : public ::testing::Test
{
protected:
    void SetUp() override;

protected:
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    const Utf8String filePath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp");
    const Utf8String otherFilePath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.h");
    const Utf8String headerPath = Utf8StringLiteral(TESTDATA_DIR"/translationunits.h");
    const Utf8String nonExistingFilePath = Utf8StringLiteral("foo.cpp");
    const Utf8String projectPartId = Utf8StringLiteral("projectPartId");
    const Utf8String otherProjectPartId = Utf8StringLiteral("otherProjectPartId");
    const Utf8String nonExistingProjectPartId = Utf8StringLiteral("nonExistingProjectPartId");
    const ClangBackEnd::FileContainer fileContainer{filePath, projectPartId};
    const ClangBackEnd::FileContainer headerContainer{headerPath, projectPartId};
};

using DocumentsSlowTest = Documents;

TEST_F(Documents, ThrowForGettingWithWrongFilePath)
{
    ASSERT_THROW(documents.document(nonExistingFilePath, projectPartId),
                 ClangBackEnd::DocumentDoesNotExistException);

}

TEST_F(Documents, ThrowForGettingWithWrongProjectPartFilePath)
{
    ASSERT_THROW(documents.document(filePath, nonExistingProjectPartId),
                 ClangBackEnd::ProjectPartDoNotExistException);

}

TEST_F(Documents, ThrowForAddingNonExistingFile)
{
    ClangBackEnd::FileContainer fileContainer(nonExistingFilePath, projectPartId);

    ASSERT_THROW(documents.create({fileContainer}),
                 ClangBackEnd::DocumentFileDoesNotExistException);
}

TEST_F(Documents, DoNotThrowForAddingNonExistingFileWithUnsavedContent)
{
    ClangBackEnd::FileContainer fileContainer(nonExistingFilePath, projectPartId, Utf8String(), true);

    ASSERT_NO_THROW(documents.create({fileContainer}));
}

TEST_F(Documents, Add)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);

    documents.create({fileContainer});

    ASSERT_THAT(documents.document(filePath, projectPartId),
                IsDocument(filePath, projectPartId, 74u));
}

TEST_F(Documents, CreateWithUnsavedContentSetsDependenciesDirty)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer fileContainerWithUnsavedContent(otherFilePath, projectPartId, Utf8StringVector(), Utf8String(), true, 2u);
    auto dependentDocument = documents.create({fileContainer}).at(0);
    dependentDocument.setDependedFilePaths(QSet<Utf8String>() << filePath << otherFilePath);

    documents.create({fileContainerWithUnsavedContent});

    ASSERT_TRUE(dependentDocument.isNeedingReparse());
}

TEST_F(Documents, AddAndTestCreatedTranslationUnit)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);

    auto createdDocuments = documents.create({fileContainer});

    ASSERT_THAT(createdDocuments.front(),
                IsDocument(filePath, projectPartId, 74u));
}

TEST_F(Documents, ThrowForCreatingAnExistingDocument)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    documents.create({fileContainer});

    ASSERT_THROW(documents.create({fileContainer}),
                 ClangBackEnd::DocumentAlreadyExistsException);
}

TEST_F(Documents, ThrowForUpdatingANonExistingDocument)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    ASSERT_THROW(documents.update({fileContainer}),
                 ClangBackEnd::DocumentDoesNotExistException);
}

TEST_F(Documents, UpdateSingle)
{
    ClangBackEnd::FileContainer createFileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer updateFileContainer(filePath, Utf8String(), Utf8StringVector(), 75u);
    documents.create({createFileContainer});

    documents.update({updateFileContainer});

    ASSERT_THAT(documents.document(filePath, projectPartId),
                IsDocument(filePath, projectPartId, 75u));
}

TEST_F(Documents, UpdateReturnsUpdatedDocument)
{
    ClangBackEnd::FileContainer createFileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer updateFileContainer(filePath, Utf8String(), Utf8StringVector(), 75u);
    documents.create({createFileContainer});

    const std::vector<Document> updatedDocuments = documents.update({updateFileContainer});

    ASSERT_THAT(updatedDocuments.size(), Eq(1u));
    ASSERT_THAT(updatedDocuments.front().documentRevision(), Eq(75u));
}

TEST_F(Documents, UpdateMultiple)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer fileContainerWithOtherProject(filePath, otherProjectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer updatedFileContainer(filePath, Utf8String(), Utf8StringVector(), 75u);
    documents.create({fileContainer, fileContainerWithOtherProject});

    documents.update({updatedFileContainer});

    ASSERT_THAT(documents.document(filePath, projectPartId),
                IsDocument(filePath, projectPartId, 75u));
    ASSERT_THAT(documents.document(filePath, otherProjectPartId),
                IsDocument(filePath, otherProjectPartId, 75u));
}

TEST_F(DocumentsSlowTest, UpdateUnsavedFileAndCheckForReparse)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer headerContainer(headerPath, projectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer headerContainerWithUnsavedContent(headerPath, projectPartId, Utf8String(), true, 75u);
    documents.create({fileContainer, headerContainer});
    Document document = documents.document(filePath, projectPartId);
    document.parse();

    documents.update({headerContainerWithUnsavedContent});

    ASSERT_TRUE(documents.document(filePath, projectPartId).isNeedingReparse());
}

TEST_F(DocumentsSlowTest, RemoveFileAndCheckForReparse)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer headerContainer(headerPath, projectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer headerContainerWithUnsavedContent(headerPath, projectPartId, Utf8String(), true, 75u);
    documents.create({fileContainer, headerContainer});
    Document document = documents.document(filePath, projectPartId);
    document.parse();

    documents.remove({headerContainerWithUnsavedContent});

    ASSERT_TRUE(documents.document(filePath, projectPartId).isNeedingReparse());
}

TEST_F(Documents, DontGetNewerFileContainerIfRevisionIsTheSame)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    documents.create({fileContainer});

    auto newerFileContainers = documents.newerFileContainers({fileContainer});

    ASSERT_THAT(newerFileContainers.size(), 0);
}

TEST_F(Documents, GetNewerFileContainerIfRevisionIsDifferent)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74u);
    ClangBackEnd::FileContainer newerContainer(filePath, projectPartId, Utf8StringVector(), 75u);
    documents.create({fileContainer});

    auto newerFileContainers = documents.newerFileContainers({newerContainer});

    ASSERT_THAT(newerFileContainers.size(), 1);
}

TEST_F(Documents, ThrowForRemovingWithWrongFilePath)
{
    ClangBackEnd::FileContainer fileContainer(nonExistingFilePath, projectPartId);

    ASSERT_THROW(documents.remove({fileContainer}),
                 ClangBackEnd::DocumentDoesNotExistException);
}

TEST_F(Documents, ThrowForRemovingWithWrongProjectPartFilePath)
{
    ClangBackEnd::FileContainer fileContainer(filePath, nonExistingProjectPartId);

    ASSERT_THROW(documents.remove({fileContainer}),
                 ClangBackEnd::ProjectPartDoNotExistException);
}

TEST_F(Documents, Remove)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId);
    documents.create({fileContainer});

    documents.remove({fileContainer});

    ASSERT_THROW(documents.document(filePath, projectPartId),
                 ClangBackEnd::DocumentDoesNotExistException);
}

TEST_F(Documents, RemoveAllValidIfExceptionIsThrown)
{
    ClangBackEnd::FileContainer fileContainer(filePath, projectPartId);
    documents.create({fileContainer});

    ASSERT_THROW(documents.remove({ClangBackEnd::FileContainer(Utf8StringLiteral("dontextist.pro"), projectPartId), fileContainer}),
                 ClangBackEnd::DocumentDoesNotExistException);

    ASSERT_THAT(documents.documents(),
                Not(Contains(Document(filePath,
                                             projects.project(projectPartId),
                                             Utf8StringVector(),
                                             documents))));
}

TEST_F(Documents, HasDocument)
{
    documents.create({{filePath, projectPartId}});

    ASSERT_TRUE(documents.hasDocument(filePath, projectPartId));
}

TEST_F(Documents, HasNotDocument)
{
    ASSERT_FALSE(documents.hasDocument(filePath, projectPartId));
}

TEST_F(Documents, FilteredPositive)
{
    documents.create({{filePath, projectPartId}});
    const auto isMatchingFilePath = [this](const Document &document) {
        return document.filePath() == filePath;
    };

    const bool hasMatches = !documents.filtered(isMatchingFilePath).empty();

    ASSERT_TRUE(hasMatches);
}

TEST_F(Documents, FilteredNegative)
{
    documents.create({{filePath, projectPartId}});
    const auto isMatchingNothing = [](const Document &) {
        return false;
    };

    const bool hasMatches = !documents.filtered(isMatchingNothing).empty();

    ASSERT_FALSE(hasMatches);
}

TEST_F(Documents, DirtyAndVisibleButNotCurrentDocuments)
{
    documents.create({{filePath, projectPartId}});
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

void Documents::SetUp()
{
    projects.createOrUpdate({ProjectPartContainer(projectPartId)});
    projects.createOrUpdate({ProjectPartContainer(otherProjectPartId)});
}

}
