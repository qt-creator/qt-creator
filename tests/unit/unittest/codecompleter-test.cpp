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

#include <codecompleter.h>
#include <clangdocument.h>
#include <filecontainer.h>
#include <clangdocuments.h>
#include <unsavedfiles.h>
#include <utf8stringvector.h>

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

using ::testing::ElementsAreArray;
using ::testing::Contains;
using ::testing::AllOf;
using ::testing::Not;
using ::testing::PrintToString;

using ClangBackEnd::CodeCompletion;
using ClangBackEnd::CodeCompletionChunk;
using ClangBackEnd::CodeCompleter;

namespace {

MATCHER_P2(IsCodeCompletion, text, completionKind,
           std::string(negation ? "isn't" : "is") + " code completion with text "
           + PrintToString(text) + " and kind " + PrintToString(completionKind)
           )
{
    if (arg.text != text) {
        *result_listener << "text is " + PrintToString(arg.text) + " and not " +  PrintToString(text);
        return false;
    }

    if (arg.completionKind != completionKind) {
        *result_listener << "kind is " + PrintToString(arg.completionKind) + " and not " +  PrintToString(completionKind);
        return false;
    }

    return true;
}

MATCHER_P(IsOverloadCompletion, text,
          std::string(negation ? "isn't" : "is") + " overload completion with text " + PrintToString(text))
{
    Utf8String overloadName;
    for (auto &chunk : arg.chunks) {
        if (chunk.kind == CodeCompletionChunk::Text) {
            overloadName = chunk.text;
            break;
        }
    }
    if (overloadName != text) {
        *result_listener << "text is " + PrintToString(overloadName) + " and not " +  PrintToString(text);
        return false;
    }

    if (arg.completionKind != CodeCompletion::FunctionOverloadCompletionKind) {
        *result_listener << "kind is " + PrintToString(arg.completionKind) + " and not " +  PrintToString(CodeCompletion::FunctionOverloadCompletionKind);
        return false;
    }

    return true;
}

MATCHER(HasFixIts, "")
{
    return !arg.requiredFixIts.empty();
}

class CodeCompleter : public ::testing::Test
{
protected:
    void SetUp();
    void copyTargetHeaderToTemporaryIncludeDirecory();
    void copyChangedTargetHeaderToTemporaryIncludeDirecory();
    ClangBackEnd::CodeCompleter setupCompleter(const ClangBackEnd::FileContainer &fileContainer);
    static Utf8String readFileContent(const QString &fileName);

protected:
    QTemporaryDir includeDirectory;
    Utf8String includePathArgument{QStringLiteral("-I") + includeDirectory.path()};
    QString targetHeaderPath{includeDirectory.path() + QStringLiteral("/complete_target_header.h")};
    ClangBackEnd::FileContainer mainFileContainer{Utf8StringLiteral(TESTDATA_DIR
                                                                    "/complete_completer_main.cpp"),
                                                  Utf8StringVector{includePathArgument},
                                                  {}};
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{unsavedFiles};
    ClangBackEnd::Document document;
    QScopedPointer<ClangBackEnd::CodeCompleter> completer;
    ClangBackEnd::FileContainer unsavedMainFileContainer{mainFileContainer.filePath,
                                                         {includePathArgument},
                                                         {},
                                                         readFileContent("/complete_completer_main_unsaved.cpp"),
                                                         true};
    ClangBackEnd::FileContainer unsavedTargetHeaderFileContainer{targetHeaderPath,
                                                                 {includePathArgument},
                                                                 {},
                                                                 readFileContent("/complete_target_header_unsaved.h"),
                                                                 true};

    ClangBackEnd::FileContainer arrowFileContainer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_arrow.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_arrow.cpp"),
        true
    };
    ClangBackEnd::FileContainer dotArrowCorrectionForPointerFileContainer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withDotArrowCorrectionForPointer.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withDotArrowCorrectionForPointer.cpp"),
        true
    };
    ClangBackEnd::FileContainer dotArrowCorrectionForPointerFileContainerBeforeTyping{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withDotArrowCorrectionForPointer.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withDotArrowCorrectionForPointer_beforeTyping.cpp"),
        true
    };
    ClangBackEnd::FileContainer dotArrowCorrectionForPointerFileContainerAfterTyping{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withDotArrowCorrectionForPointer.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withDotArrowCorrectionForPointer_afterTyping.cpp"),
        true
    };
    ClangBackEnd::FileContainer dotArrowCorrectionForPointerFileContainerInitial{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withDotArrowCorrectionForPointer.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withDotArrowCorrectionForPointerInitial.cpp"),
        true
    };
    ClangBackEnd::FileContainer dotArrowCorrectionForPointerFileContainerUpdated{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withDotArrowCorrectionForPointer.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withDotArrowCorrectionForPointerUpdated.cpp"),
        true
    };
    ClangBackEnd::FileContainer noDotArrowCorrectionForObjectFileContainer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withNoDotArrowCorrectionForObject.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withNoDotArrowCorrectionForObject.cpp"),
        true
    };
    ClangBackEnd::FileContainer noDotArrowCorrectionForFloatFileContainer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withNoDotArrowCorrectionForFloat.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withNoDotArrowCorrectionForFloat.cpp"),
        true
    };
    ClangBackEnd::FileContainer noDotArrowCorrectionForObjectWithArrowOperatortFileContainer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withNoDotArrowCorrectionForObjectWithArrowOperator.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withNoDotArrowCorrectionForObjectWithArrowOperator.cpp"),
        true
    };
    ClangBackEnd::FileContainer noDotArrowCorrectionForDotDotFileContainer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withNoDotArrowCorrectionForDotDot.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withNoDotArrowCorrectionForDotDot.cpp"),
        true
    };
    ClangBackEnd::FileContainer noDotArrowCorrectionForArrowDotFileContainer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withNoDotArrowCorrectionForArrowDot.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withNoDotArrowCorrectionForArrowDot.cpp"),
        true
    };
    ClangBackEnd::FileContainer noDotArrowCorrectionForOnlyDotFileContainer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withNoDotArrowCorrectionForOnlyDot.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withNoDotArrowCorrectionForOnlyDot.cpp"),
        true
    };
    ClangBackEnd::FileContainer noDotArrowCorrectionForColonColonFileContainer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withNoDotArrowCorrectionForColonColon.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withNoDotArrowCorrectionForColonColon.cpp"),
        true
    };
    ClangBackEnd::FileContainer dotArrowCorrectionForForwardDeclaredClassPointer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withDotArrowCorrectionForForwardDeclaredClassPointer.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withDotArrowCorrectionForForwardDeclaredClassPointer.cpp"),
        true
    };
    ClangBackEnd::FileContainer globalCompletionAfterForwardDeclaredClassPointer{
        Utf8StringLiteral(TESTDATA_DIR"/complete_withGlobalCompletionAfterForwardDeclaredClassPointer.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_withGlobalCompletionAfterForwardDeclaredClassPointer.cpp"),
        true
    };
    ClangBackEnd::FileContainer smartPointerCompletion{
        Utf8StringLiteral(TESTDATA_DIR"/complete_smartpointer.cpp"),
        {includePathArgument},
        {},
        readFileContent("/complete_smartpointer.cpp"),
        true
    };
    ClangBackEnd::FileContainer completionsOrder{
        Utf8StringLiteral(TESTDATA_DIR"/completions_order.cpp"),
        {includePathArgument},
        {},
        readFileContent("/completions_order.cpp"),
        true
    };
};

using CodeCompleterSlowTest = CodeCompleter;

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
    bool hasCopied = QFile::copy(QString::fromUtf8(TESTDATA_DIR "/complete_target_header.h"),
                                 targetHeaderPath);
    EXPECT_TRUE(hasCopied);
}

void CodeCompleter::copyChangedTargetHeaderToTemporaryIncludeDirecory()
{
    QFile::remove(targetHeaderPath);
    bool hasCopied = QFile::copy(QString::fromUtf8(TESTDATA_DIR "/complete_target_header_changed.h"),
                                 targetHeaderPath);
    EXPECT_TRUE(hasCopied);
}

void CodeCompleter::SetUp()
{
    EXPECT_TRUE(includeDirectory.isValid());
    documents.create({mainFileContainer});
    document = documents.document(mainFileContainer);
    completer.reset(new ClangBackEnd::CodeCompleter(document.translationUnit(), unsavedFiles));

    copyTargetHeaderToTemporaryIncludeDirecory();
    document.parse();
}

TEST_F(CodeCompleterSlowTest, FunctionInUnsavedFile)
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});
    documents.update({unsavedMainFileContainer});
    ClangBackEnd::CodeCompleter myCompleter(document.translationUnit(), unsavedFiles);

    ASSERT_THAT(myCompleter.complete(27, 1),
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

TEST_F(CodeCompleterSlowTest, VariableInUnsavedFile)
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});
    documents.update({unsavedMainFileContainer});
    ClangBackEnd::CodeCompleter myCompleter(document.translationUnit(), unsavedFiles);

    ASSERT_THAT(myCompleter.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("VariableInUnsavedFile"),
                                          CodeCompletion::VariableCompletionKind)));
}

TEST_F(CodeCompleterSlowTest, GlobalVariableInUnsavedFile)
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});
    documents.update({unsavedMainFileContainer});
    ClangBackEnd::CodeCompleter myCompleter(document.translationUnit(), unsavedFiles);

    ASSERT_THAT(myCompleter.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("GlobalVariableInUnsavedFile"),
                                          CodeCompletion::VariableCompletionKind)));
}

TEST_F(CodeCompleterSlowTest, Macro)
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});
    documents.update({unsavedMainFileContainer});
    ClangBackEnd::CodeCompleter myCompleter(document.translationUnit(), unsavedFiles);

    ASSERT_THAT(myCompleter.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("Macro"),
                                          CodeCompletion::PreProcessorCompletionKind)));
}

TEST_F(CodeCompleterSlowTest, Keyword)
{
    ASSERT_THAT(completer->complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("switch"),
                                          CodeCompletion::KeywordCompletionKind)));
}

TEST_F(CodeCompleterSlowTest, FunctionInIncludedHeader)
{
    ASSERT_THAT(completer->complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("FunctionInIncludedHeader"),
                                          CodeCompletion::FunctionCompletionKind)));
}

TEST_F(CodeCompleterSlowTest, UniquePointerCompletion)
{
    auto myCompleter = setupCompleter(smartPointerCompletion);

    ASSERT_THAT(myCompleter.complete(59, 54, 59, 32),
                Contains(IsOverloadCompletion(Utf8StringLiteral("Bar"))));
}

TEST_F(CodeCompleterSlowTest, SharedPointerCompletion)
{
    auto myCompleter = setupCompleter(smartPointerCompletion);

    ASSERT_THAT(myCompleter.complete(60, 55, 60, 33),
                Contains(IsOverloadCompletion(Utf8StringLiteral("Bar"))));
}

TEST_F(CodeCompleterSlowTest, QSharedPointerCompletion)
{
    auto myCompleter = setupCompleter(smartPointerCompletion);

    ASSERT_THAT(myCompleter.complete(61, 60, 61, 32),
                Contains(IsOverloadCompletion(Utf8StringLiteral("Bar"))));
}

TEST_F(CodeCompleterSlowTest, FunctionInUnsavedIncludedHeader)
{
    unsavedFiles.createOrUpdate({unsavedTargetHeaderFileContainer});
    documents.create({unsavedTargetHeaderFileContainer});
    ClangBackEnd::CodeCompleter myCompleter(document.translationUnit(), unsavedFiles);

    ASSERT_THAT(myCompleter.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("FunctionInIncludedHeaderUnsaved"),
                                          CodeCompletion::FunctionCompletionKind)));
}

TEST_F(CodeCompleterSlowTest, DISABLED_FunctionInChangedIncludedHeader)
{
    copyChangedTargetHeaderToTemporaryIncludeDirecory();

    ASSERT_THAT(completer->complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("FunctionInIncludedHeaderChanged"),
                                          CodeCompletion::FunctionCompletionKind)));
}

TEST_F(CodeCompleterSlowTest, DISABLED_FunctionInChangedIncludedHeaderWithUnsavedContentInMainFile) // it's not that bad because we reparse anyway
{
    unsavedFiles.createOrUpdate({unsavedMainFileContainer});
    documents.update({unsavedMainFileContainer});
    ClangBackEnd::CodeCompleter myCompleter(document.translationUnit(), unsavedFiles);

    copyChangedTargetHeaderToTemporaryIncludeDirecory();

    ASSERT_THAT(myCompleter.complete(27, 1),
                Contains(IsCodeCompletion(Utf8StringLiteral("FunctionInIncludedHeaderChanged"),
                                          CodeCompletion::FunctionCompletionKind)));
}

TEST_F(CodeCompleterSlowTest, ArrowCompletion)
{
    auto myCompleter = setupCompleter(arrowFileContainer);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(5, 10);

    ASSERT_THAT(completions,
                Contains(IsCodeCompletion(Utf8StringLiteral("member"),
                                          CodeCompletion::VariableCompletionKind)));
    ASSERT_THAT(completions, Not(Contains(HasFixIts())));
}

TEST_F(CodeCompleterSlowTest, DotToArrowCompletionForPointer)
{
    auto myCompleter = setupCompleter(dotArrowCorrectionForPointerFileContainer);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(5, 9);

    ASSERT_THAT(completions,
                Contains(IsCodeCompletion(Utf8StringLiteral("member"),
                                          CodeCompletion::VariableCompletionKind)));
    ASSERT_THAT(completions, Contains(HasFixIts()));
}

TEST_F(CodeCompleterSlowTest, DotToArrowCompletionForPointerInOutdatedDocument)
{
    auto fileContainerBeforeTyping = dotArrowCorrectionForPointerFileContainerBeforeTyping;
    documents.create({fileContainerBeforeTyping});
    unsavedFiles.createOrUpdate({fileContainerBeforeTyping});
    auto document = documents.document(fileContainerBeforeTyping.filePath);
    document.parse();
    unsavedFiles.createOrUpdate({dotArrowCorrectionForPointerFileContainerAfterTyping});
    ClangBackEnd::CodeCompleter myCompleter(documents.document(dotArrowCorrectionForPointerFileContainerAfterTyping).translationUnit(),
                                            unsavedFiles);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(5, 9);

    ASSERT_THAT(completions,
                Contains(IsCodeCompletion(Utf8StringLiteral("member"),
                                          CodeCompletion::VariableCompletionKind)));
    ASSERT_THAT(completions, Contains(HasFixIts()));
}

TEST_F(CodeCompleterSlowTest, NoDotToArrowCompletionForObject)
{
    auto myCompleter = setupCompleter(noDotArrowCorrectionForObjectFileContainer);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(5, 9);

    ASSERT_THAT(completions,
                Contains(IsCodeCompletion(Utf8StringLiteral("member"),
                                          CodeCompletion::VariableCompletionKind)));
    ASSERT_THAT(completions, Not(Contains(HasFixIts())));
}

TEST_F(CodeCompleterSlowTest, NoDotToArrowCompletionForFloat)
{
    auto myCompleter = setupCompleter(noDotArrowCorrectionForFloatFileContainer);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(3, 18);

    ASSERT_TRUE(completions.isEmpty());
}

TEST_F(CodeCompleterSlowTest, NoDotArrowCorrectionForObjectWithArrowOperator)
{
    auto myCompleter = setupCompleter(noDotArrowCorrectionForObjectWithArrowOperatortFileContainer);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(8, 9);

    ASSERT_THAT(completions,
                Contains(IsCodeCompletion(Utf8StringLiteral("member"),
                                          CodeCompletion::VariableCompletionKind)));
    ASSERT_THAT(completions, Not(Contains(HasFixIts())));
}

TEST_F(CodeCompleterSlowTest, NoDotArrowCorrectionForDotDot)
{
    auto myCompleter = setupCompleter(noDotArrowCorrectionForDotDotFileContainer);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(5, 10);

    ASSERT_TRUE(completions.isEmpty());
}

TEST_F(CodeCompleterSlowTest, NoDotArrowCorrectionForArrowDot)
{
    auto myCompleter = setupCompleter(noDotArrowCorrectionForArrowDotFileContainer);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(5, 11);

    ASSERT_TRUE(completions.isEmpty());
}

TEST_F(CodeCompleterSlowTest, NoDotArrowCorrectionForOnlyDot)
{
    auto myCompleter = setupCompleter(noDotArrowCorrectionForOnlyDotFileContainer);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(5, 6);

    ASSERT_TRUE(completions.isEmpty());
}

TEST_F(CodeCompleterSlowTest, GlobalCompletionForSpaceAfterOnlyDot)
{
    auto myCompleter = setupCompleter(noDotArrowCorrectionForOnlyDotFileContainer);

    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(5, 7);
    ASSERT_THAT(completions,
                Contains(IsCodeCompletion(Utf8StringLiteral("Foo"),
                                          CodeCompletion::ClassCompletionKind)));
}

TEST_F(CodeCompleterSlowTest, NoDotArrowCorrectionForColonColon)
{
    auto myCompleter = setupCompleter(noDotArrowCorrectionForColonColonFileContainer);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(1, 7);

    ASSERT_THAT(completions, Not(Contains(HasFixIts())));
}

TEST_F(CodeCompleterSlowTest, NoGlobalCompletionAfterForwardDeclaredClassPointer)
{
    auto myCompleter = setupCompleter(globalCompletionAfterForwardDeclaredClassPointer);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(5, 10);

    ASSERT_TRUE(completions.isEmpty());
}

TEST_F(CodeCompleterSlowTest, GlobalCompletionAfterForwardDeclaredClassPointer)
{
    auto myCompleter = setupCompleter(globalCompletionAfterForwardDeclaredClassPointer);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(6, 4);

    ASSERT_TRUE(!completions.isEmpty());
}

TEST_F(CodeCompleterSlowTest, ConstructorCompletionExists)
{
    auto myCompleter = setupCompleter(completionsOrder);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(8, 1);

    int constructorIndex = Utils::indexOf(completions, [](const CodeCompletion &codeCompletion) {
        return codeCompletion.text == "Constructor" && codeCompletion.completionKind == CodeCompletion::ConstructorCompletionKind;
    });

    ASSERT_THAT(constructorIndex != -1, true);
}

TEST_F(CodeCompleterSlowTest, ClassConstructorCompletionsOrder)
{
    auto myCompleter = setupCompleter(completionsOrder);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(8, 1);

    int classIndex = Utils::indexOf(completions, [](const CodeCompletion &codeCompletion) {
        return codeCompletion.text == "Constructor" && codeCompletion.completionKind == CodeCompletion::ClassCompletionKind;
    });
    int constructorIndex = Utils::indexOf(completions, [](const CodeCompletion &codeCompletion) {
        return codeCompletion.text == "Constructor" && codeCompletion.completionKind == CodeCompletion::ConstructorCompletionKind;
    });

    ASSERT_THAT(classIndex < constructorIndex, true);
}

TEST_F(CodeCompleterSlowTest, SharedPointerCompletionsOrder)
{
    auto myCompleter = setupCompleter(smartPointerCompletion);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(62, 11);

    int resetIndex = Utils::indexOf(completions, [](const CodeCompletion &codeCompletion) {
        return codeCompletion.text == "reset";
    });
    int barDestructorIndex = Utils::indexOf(completions, [](const CodeCompletion &codeCompletion) {
        return codeCompletion.text == "~Bar";
    });

    ASSERT_THAT(barDestructorIndex < resetIndex, true);
}

TEST_F(CodeCompleterSlowTest, ConstructorHasOverloadCompletions)
{
    auto myCompleter = setupCompleter(completionsOrder);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(8, 1);

    int constructorsCount = Utils::count(completions, [](const CodeCompletion &codeCompletion) {
        return codeCompletion.text == "Constructor" && codeCompletion.completionKind == CodeCompletion::ConstructorCompletionKind;
    });

    ASSERT_THAT(constructorsCount, 2);
}

TEST_F(CodeCompleterSlowTest, FunctionOverloadsNoParametersOrder)
{
    auto myCompleter = setupCompleter(completionsOrder);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(27, 7);

    int firstIndex = Utils::indexOf(completions, [](const CodeCompletion &codeCompletion) {
        return codeCompletion.text == "foo";
    });
    int secondIndex = Utils::indexOf(completions, [i = 0, firstIndex](const CodeCompletion &codeCompletion) mutable {
        return (i++) > firstIndex && codeCompletion.text == "foo";
    });

    ASSERT_THAT(abs(firstIndex - secondIndex), 1);
}

TEST_F(CodeCompleterSlowTest, FunctionOverloadsWithParametersOrder)
{
    auto myCompleter = setupCompleter(completionsOrder);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(27, 7);

    int firstIndex = Utils::indexOf(completions, [](const CodeCompletion &codeCompletion) {
        return codeCompletion.text == "bar";
    });
    int secondIndex = Utils::indexOf(completions, [i = 0, firstIndex](const CodeCompletion &codeCompletion) mutable {
        return (i++) > firstIndex && codeCompletion.text == "bar";
    });

    ASSERT_THAT(abs(firstIndex - secondIndex), 1);
}

TEST_F(CodeCompleterSlowTest, FunctionOverloadsWithoutDotOrArrowOrder)
{
    auto myCompleter = setupCompleter(completionsOrder);
    const ClangBackEnd::CodeCompletions completions = myCompleter.complete(21, 1);

    int firstIndex = Utils::indexOf(completions, [](const CodeCompletion &codeCompletion) {
        return codeCompletion.text == "bar";
    });
    int secondIndex = Utils::indexOf(completions, [i = 0, firstIndex](const CodeCompletion &codeCompletion) mutable {
        return (i++) > firstIndex && codeCompletion.text == "bar";
    });

    ASSERT_THAT(abs(firstIndex - secondIndex), 1);
}

ClangBackEnd::CodeCompleter CodeCompleter::setupCompleter(
        const ClangBackEnd::FileContainer &fileContainer)
{
    documents.create({fileContainer});
    unsavedFiles.createOrUpdate({fileContainer});
    document = documents.document(fileContainer);
    document.parse();

    ClangBackEnd::Document document = documents.document(fileContainer);
    return ClangBackEnd::CodeCompleter(document.translationUnit(),
                                       unsavedFiles);
}

}
