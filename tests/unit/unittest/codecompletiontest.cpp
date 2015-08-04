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

#include <codecompleter.h>
#include <filecontainer.h>
#include <projectpart.h>
#include <translationunit.h>
#include <unsavedfiles.h>
#include <utf8stringvector.h>

#include <QFile>
#include <QTemporaryDir>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ::testing::ElementsAreArray;
using ::testing::Contains;
using ::testing::AllOf;
using ::testing::Not;
using ::testing::PrintToString;

using ClangBackEnd::CodeCompletion;
using ClangBackEnd::CodeCompleter;

namespace {

MATCHER_P2(IsCodeCompletion, text, completionKind,
           std::string(negation ? "isn't" : "is") + " code completion with text "
           + PrintToString(text) + " and kind " + PrintToString(completionKind)
           )
{
    if (arg.text() != text) {
        *result_listener << "text is " + PrintToString(arg.text()) + " and not " +  PrintToString(text);
        return false;
    }

    if (arg.completionKind() != completionKind) {
        *result_listener << "kind is " + PrintToString(arg.completionKind()) + " and not " +  PrintToString(completionKind);
        return false;
    }

    return true;
}

class CodeCompleter : public ::testing::Test
{
protected:
    void SetUp();
    void copyTargetHeaderToTemporaryIncludeDirecory();
    void copyChangedTargetHeaderToTemporaryIncludeDirecory();
    static Utf8String readFileContent(const QString &fileName);

protected:
    QTemporaryDir includeDirectory;
    QString targetHeaderPath{includeDirectory.path() + QStringLiteral("/complete_target_header.h")};
    ClangBackEnd::ProjectPart projectPart{Utf8StringLiteral("projectPartId")};
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnit translationUnit{Utf8StringLiteral(TESTDATA_DIR"/complete_completer_main.cpp"),
                                                  unsavedFiles,
                                                  projectPart};
    ClangBackEnd::CodeCompleter completer{translationUnit};
    ClangBackEnd::FileContainer unsavedMainFileContainer{translationUnit.filePath(),
                                                         projectPart.projectPartId(),
                                                         readFileContent(QStringLiteral("/complete_completer_main_unsaved.cpp")),
                                                         true};
    ClangBackEnd::FileContainer unsavedTargetHeaderFileContainer{targetHeaderPath,
                                                                 projectPart.projectPartId(),
                                                                 readFileContent(QStringLiteral("/complete_target_header_unsaved.h")),
                                                                 true};
};

Utf8String CodeCompleter::readFileContent(const QString &fileName)
{
    QFile readFileContentFile(QStringLiteral(TESTDATA_DIR) + fileName);
    bool hasOpened = readFileContentFile.open(QIODevice::ReadOnly | QIODevice::Text);

    EXPECT_TRUE(hasOpened);

    return Utf8String::fromByteArray(readFileContentFile.readAll());
}

void CodeCompleter::copyTargetHeaderToTemporaryIncludeDirecory()
{
    QFile::remove(targetHeaderPath);
    bool hasCopied = QFile::copy(QStringLiteral(TESTDATA_DIR "/complete_target_header.h"),
                                 targetHeaderPath);
    EXPECT_TRUE(hasCopied);
}

void CodeCompleter::copyChangedTargetHeaderToTemporaryIncludeDirecory()
{
    QFile::remove(targetHeaderPath);
    bool hasCopied = QFile::copy(QStringLiteral(TESTDATA_DIR "/complete_target_header_changed.h"),
                                 targetHeaderPath);
    EXPECT_TRUE(hasCopied);
}

void CodeCompleter::SetUp()
{
    EXPECT_TRUE(includeDirectory.isValid());

    Utf8String includePath(QStringLiteral("-I") + includeDirectory.path());
    projectPart.setArguments({includePath});

    copyTargetHeaderToTemporaryIncludeDirecory();

    translationUnit.cxTranslationUnit(); // initialize translation unit so we check changes
}

TEST_F(CodeCompleter, FunctionInUnsavedFile)
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});

    ASSERT_THAT(completer.complete(27, 1),
                AllOf(Contains(IsCodeCompletion(Utf8StringLiteral("FunctionWithArguments"),
                                                CodeCompletion::FunctionCompletionKind)),
                      Contains(IsCodeCompletion(Utf8StringLiteral("Function"),
                                                CodeCompletion::FunctionCompletionKind)),
                      Contains(IsCodeCompletion(Utf8StringLiteral("UnsavedFunction"),
                                                CodeCompletion::FunctionCompletionKind)),
                      Contains(IsCodeCompletion(Utf8StringLiteral("f"),
                                                CodeCompletion::FunctionCompletionKind)),
                      Not(Contains(IsCodeCompletion(Utf8StringLiteral("SavedFunction"),
                                                    CodeCompletion::FunctionCompletionKind)))));
}

TEST_F(CodeCompleter, VariableInUnsavedFile)
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});

    ASSERT_THAT(completer.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("VariableInUnsavedFile"),
                                          CodeCompletion::VariableCompletionKind)));
}

TEST_F(CodeCompleter, GlobalVariableInUnsavedFile)
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});

    ASSERT_THAT(completer.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("GlobalVariableInUnsavedFile"),
                                          CodeCompletion::VariableCompletionKind)));
}

TEST_F(CodeCompleter, Macro)
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});

    ASSERT_THAT(completer.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("Macro"),
                                          CodeCompletion::PreProcessorCompletionKind)));
}

TEST_F(CodeCompleter, Keyword)
{
    ASSERT_THAT(completer.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("switch"),
                                          CodeCompletion::KeywordCompletionKind)));
}

TEST_F(CodeCompleter, FunctionInIncludedHeader)
{
    ASSERT_THAT(completer.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("FunctionInIncludedHeader"),
                                          CodeCompletion::FunctionCompletionKind)));
}

TEST_F(CodeCompleter, FunctionInUnsavedIncludedHeader)
{
    unsavedFiles.createOrUpdate({unsavedTargetHeaderFileContainer});

    ASSERT_THAT(completer.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("FunctionInIncludedHeaderUnsaved"),
                                          CodeCompletion::FunctionCompletionKind)));
}

TEST_F(CodeCompleter, FunctionInChangedIncludedHeader)
{
    copyChangedTargetHeaderToTemporaryIncludeDirecory();

    ASSERT_THAT(completer.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("FunctionInIncludedHeaderChanged"),
                                          CodeCompletion::FunctionCompletionKind)));
}

TEST_F(CodeCompleter, FunctionInChangedIncludedHeaderWithUnsavedContentInMainFile)
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});

    copyChangedTargetHeaderToTemporaryIncludeDirecory();

    ASSERT_THAT(completer.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("FunctionInIncludedHeaderChanged"),
                                          CodeCompletion::FunctionCompletionKind)));
}

}
