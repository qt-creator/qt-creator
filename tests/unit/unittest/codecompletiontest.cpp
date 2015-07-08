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

#include "gtest/gtest.h"
#include "gmock/gmock-matchers.h"
#include "gtest-qt-printing.h"

#include <codecompleter.h>
#include <filecontainer.h>
#include <translationunit.h>
#include <unsavedfiles.h>
#include <utf8stringvector.h>
#include <projectpart.h>

#include <QFile>


using ::testing::ElementsAreArray;
using ::testing::Contains;
using ::testing::AllOf;
using ::testing::Not;

using ClangBackEnd::CodeCompletion;
using ClangBackEnd::CodeCompleter;

namespace {

class CodeCompleter : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static ClangBackEnd::ProjectPart projectPart;
    static ClangBackEnd::UnsavedFiles unsavedFiles;
    static ClangBackEnd::TranslationUnit translationUnit;
    static ClangBackEnd::CodeCompleter completer;
};

ClangBackEnd::ProjectPart CodeCompleter::projectPart(Utf8StringLiteral("projectPartId"));
ClangBackEnd::UnsavedFiles CodeCompleter::unsavedFiles;
ClangBackEnd::TranslationUnit CodeCompleter::translationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_completer.cpp"), unsavedFiles, projectPart);
ClangBackEnd::CodeCompleter CodeCompleter::completer = translationUnit;

void CodeCompleter::SetUpTestCase()
{
    QFile unsavedFileContentFile(QStringLiteral(TESTDATA_DIR) + QStringLiteral("/complete_completer_unsaved.cpp"));
    unsavedFileContentFile.open(QIODevice::ReadOnly);

    const Utf8String unsavedFileContent = Utf8String::fromByteArray(unsavedFileContentFile.readAll());
    const ClangBackEnd::FileContainer unsavedDataFileContainer(translationUnit.filePath(),
                                                                   projectPart.projectPartId(),
                                                                   unsavedFileContent,
                                                                   true);

    unsavedFiles.createOrUpdate({unsavedDataFileContainer});
}

void CodeCompleter::TearDownTestCase()
{
    translationUnit.reset();
    completer = ClangBackEnd::CodeCompleter();
}


TEST_F(CodeCompleter, FunctionInUnsavedFile)
{
    ASSERT_THAT(completer.complete(100, 1),
                AllOf(Contains(CodeCompletion(Utf8StringLiteral("functionWithArguments"),
                                              Utf8String(),
                                              Utf8String(),
                                              0,
                                              CodeCompletion::FunctionCompletionKind)),
                      Contains(CodeCompletion(Utf8StringLiteral("function"),
                                              Utf8String(),
                                              Utf8String(),
                                              0,
                                              CodeCompletion::FunctionCompletionKind)),
                      Contains(CodeCompletion(Utf8StringLiteral("newFunction"),
                                              Utf8String(),
                                              Utf8String(),
                                              0,
                                              CodeCompletion::FunctionCompletionKind)),
                      Contains(CodeCompletion(Utf8StringLiteral("f"),
                                              Utf8String(),
                                              Utf8String(),
                                              0,
                                              CodeCompletion::FunctionCompletionKind)),
                      Not(Contains(CodeCompletion(Utf8StringLiteral("otherFunction"),
                                                  Utf8String(),
                                                  Utf8String(),
                                                  0,
                                                  CodeCompletion::FunctionCompletionKind)))));
}


TEST_F(CodeCompleter, Macro)
{
    ASSERT_THAT(completer.complete(100, 1),
                Contains(CodeCompletion(Utf8StringLiteral("Macro"),
                                              Utf8String(),
                                              Utf8String(),
                                              0,
                                              CodeCompletion::PreProcessorCompletionKind)));
}

TEST_F(CodeCompleter, Keyword)
{
    ASSERT_THAT(completer.complete(100, 1),
                Contains(CodeCompletion(Utf8StringLiteral("switch"),
                                              Utf8String(),
                                              Utf8String(),
                                              0,
                                              CodeCompletion::KeywordCompletionKind)));
}


}


