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

#include <clangcodecompleteresults.h>
#include <codecompletionsextractor.h>
#include <filecontainer.h>
#include <projectpart.h>
#include <translationunit.h>
#include <unsavedfiles.h>
#include <utf8stringvector.h>

#include <clang-c/Index.h>

#include <QFile>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ClangBackEnd::CodeCompletionsExtractor;
using ClangBackEnd::ClangCodeCompleteResults;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::CodeCompletion;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::CodeCompletionChunk;
using ClangBackEnd::CodeCompletionChunks;

namespace {

using ::testing::PrintToString;
using ::testing::Not;

MATCHER_P3(HasCompletion, name, kind,  availability,
           std::string(negation ? "hasn't" : "has") + " completion of name " + PrintToString(name) +
           ", kind " + PrintToString(kind))
{
    ::CodeCompletionsExtractor &extractor = const_cast<::CodeCompletionsExtractor&>(arg);
    while (extractor.next()) {
        if (extractor.currentCodeCompletion().text() == name) {
            if (extractor.currentCodeCompletion().completionKind() == kind) {
                if (extractor.currentCodeCompletion().availability() == availability) {
                    return true;
                } else if (!extractor.peek(name)) {
                    *result_listener << "availability is " << PrintToString(extractor.currentCodeCompletion().availability()) << " and not " << PrintToString(availability);
                    return false;
                }
            } else if (!extractor.peek(name)) {
                *result_listener << "kind is " << PrintToString(extractor.currentCodeCompletion().completionKind()) << " and not " << PrintToString(kind);
                return false;
            }
        }
    }

    return false;
}

MATCHER_P2(HasCompletionChunks, name, chunks,
           std::string(negation ? "hasn't" : "has") + " completion of name " + PrintToString(name) +
           " with the chunks " + PrintToString(chunks))
{
    ::CodeCompletionsExtractor &extractor = const_cast<::CodeCompletionsExtractor&>(arg);
    while (extractor.next()) {
        if (extractor.currentCodeCompletion().text() == name) {
            if (extractor.currentCodeCompletion().chunks() == chunks) {
                return true;
            } else if (!extractor.peek(name)) {
                *result_listener << "chunks are " << PrintToString(arg.currentCodeCompletion().chunks()) << " and not " << PrintToString(chunks);
                return false;
            }
        }
    }

    return false;
}

const Utf8String unsavedFileContent(const char *unsavedFilePath)
{
    QFile unsavedFileContentFile(QString::fromUtf8(unsavedFilePath));
    bool isOpen = unsavedFileContentFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!isOpen)
        ADD_FAILURE() << "File with the unsaved content cannot be opened!";

    return Utf8String::fromByteArray(unsavedFileContentFile.readAll());
}

const ClangBackEnd::FileContainer unsavedDataFileContainer(const char *filePath,
                                                           const char *unsavedFilePath)
{
    return ClangBackEnd::FileContainer(Utf8String::fromUtf8(filePath),
                                       Utf8String(),
                                       unsavedFileContent(unsavedFilePath),
                                       true);
}

ClangCodeCompleteResults getResults(const TranslationUnit &translationUnit, uint line, uint column = 1)
{
    return ClangCodeCompleteResults(clang_codeCompleteAt(translationUnit.cxTranslationUnit(),
                                                         translationUnit.filePath().constData(),
                                                         line,
                                                         column,
                                                         translationUnit.cxUnsavedFiles(),
                                                         translationUnit.unsavedFilesCount(),
                                                         CXCodeComplete_IncludeMacros | CXCodeComplete_IncludeCodePatterns));
}

class CodeCompletionsExtractor : public ::testing::Test
{
public:
    static void TearDownTestCase();

protected:
    static ClangBackEnd::ProjectPart project;
    static ClangBackEnd::UnsavedFiles unsavedFiles;
    static TranslationUnit functionTranslationUnit;
    static TranslationUnit variableTranslationUnit;
    static TranslationUnit classTranslationUnit ;
    static TranslationUnit namespaceTranslationUnit;
    static TranslationUnit enumerationTranslationUnit;
    static TranslationUnit constructorTranslationUnit;
};

ClangBackEnd::ProjectPart CodeCompletionsExtractor::project(Utf8StringLiteral("/path/to/projectfile"));
ClangBackEnd::UnsavedFiles CodeCompletionsExtractor::unsavedFiles;
TranslationUnit CodeCompletionsExtractor::functionTranslationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_function.cpp"), unsavedFiles, project);
TranslationUnit CodeCompletionsExtractor::variableTranslationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_variable.cpp"), unsavedFiles, project);
TranslationUnit CodeCompletionsExtractor::classTranslationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_class.cpp"), unsavedFiles, project);
TranslationUnit CodeCompletionsExtractor::namespaceTranslationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_namespace.cpp"), unsavedFiles, project);
TranslationUnit CodeCompletionsExtractor::enumerationTranslationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_enumeration.cpp"), unsavedFiles, project);
TranslationUnit CodeCompletionsExtractor::constructorTranslationUnit(Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_constructor.cpp"), unsavedFiles, project);

void CodeCompletionsExtractor::TearDownTestCase()
{
    functionTranslationUnit.reset();
    variableTranslationUnit.reset();
    classTranslationUnit.reset();
    namespaceTranslationUnit.reset();
    enumerationTranslationUnit.reset();
    constructorTranslationUnit.reset();
}

TEST_F(CodeCompletionsExtractor, Function)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Function"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, TemplateFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("TemplateFunction"),
                                         CodeCompletion::TemplateFunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Variable)
{
    ClangCodeCompleteResults completeResults(getResults(variableTranslationUnit, 4));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Var"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}


TEST_F(CodeCompletionsExtractor, NonTypeTemplateParameter)
{
    ClangCodeCompleteResults completeResults(getResults(variableTranslationUnit, 25, 19));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("NonTypeTemplateParameter"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}


TEST_F(CodeCompletionsExtractor, VariableReference)
{
    ClangCodeCompleteResults completeResults(getResults(variableTranslationUnit, 12));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Var"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Parameter)
{
    ClangCodeCompleteResults completeResults(getResults(variableTranslationUnit, 4));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Parameter"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Field)
{
    ClangCodeCompleteResults completeResults(getResults(variableTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Field"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Class)
{
    ClangCodeCompleteResults completeResults(getResults(classTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Class"),
                                         CodeCompletion::ClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Struct)
{
    ClangCodeCompleteResults completeResults(getResults(classTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Struct"),
                                         CodeCompletion::ClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Union)
{
    ClangCodeCompleteResults completeResults(getResults(classTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Union"),
                                         CodeCompletion::ClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, TemplateTypeParameter)
{
    ClangCodeCompleteResults completeResults(getResults(classTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("TemplateTypeParameter"),
                                         CodeCompletion::ClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, TemplateClass)
{
    ClangCodeCompleteResults completeResults(getResults(classTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("TemplateClass"),
                                         CodeCompletion::TemplateClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, TemplateTemplateParameter)
{
    ClangCodeCompleteResults completeResults(getResults(classTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("TemplateTemplateParameter"),
                                         CodeCompletion::TemplateClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, ClassTemplatePartialSpecialization)
{
    ClangCodeCompleteResults completeResults(getResults(classTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("ClassTemplatePartialSpecialization"),
                                         CodeCompletion::TemplateClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Namespace)
{
    ClangCodeCompleteResults completeResults(getResults(namespaceTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Namespace"),
                                         CodeCompletion::NamespaceCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, NamespaceAlias)
{
    ClangCodeCompleteResults completeResults(getResults(namespaceTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("NamespaceAlias"),
                                         CodeCompletion::NamespaceCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Enumeration)
{
    ClangCodeCompleteResults completeResults(getResults(enumerationTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Enumeration"),
                                         CodeCompletion::EnumerationCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Enumerator)
{
    ClangCodeCompleteResults completeResults(getResults(enumerationTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Enumerator"),
                                         CodeCompletion::EnumeratorCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, DISABLED_Constructor)
{
    ClangCodeCompleteResults completeResults(getResults(constructorTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Constructor"),
                                         CodeCompletion::ConstructorCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Destructor)
{
    ClangCodeCompleteResults completeResults(getResults(constructorTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("~Constructor"),
                                         CodeCompletion::DestructorCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Method)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Method"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
    ASSERT_FALSE(extractor.currentCodeCompletion().hasParameters());
}

TEST_F(CodeCompletionsExtractor, MethodWithParameters)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("MethodWithParameters"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
    ASSERT_TRUE(extractor.currentCodeCompletion().hasParameters());
}

TEST_F(CodeCompletionsExtractor, Slot)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Slot"),
                                         CodeCompletion::SlotCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, Signal)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Signal"),
                                         CodeCompletion::SignalCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, MacroDefinition)
{
    ClangCodeCompleteResults completeResults(getResults(variableTranslationUnit, 35));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("MacroDefinition"),
                                         CodeCompletion::PreProcessorCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, FunctionMacro)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("FunctionMacro"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, IntKeyword)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("int"),
                                         CodeCompletion::KeywordCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, SwitchKeyword)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("switch"),
                                         CodeCompletion::KeywordCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, ClassKeyword)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("class"),
                                         CodeCompletion::KeywordCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, DeprecatedFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("DeprecatedFunction"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Deprecated));
}

TEST_F(CodeCompletionsExtractor, NotAccessibleFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("NotAccessibleFunction"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::NotAccessible));
}

TEST_F(CodeCompletionsExtractor, NotAvailableFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("NotAvailableFunction"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::NotAvailable));
}

TEST_F(CodeCompletionsExtractor, UnsavedFile)
{
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::ProjectPart project(Utf8StringLiteral("/path/to/projectfile"));
    TranslationUnit translationUnit(Utf8String::fromUtf8(TESTDATA_DIR"/complete_extractor_function.cpp"), unsavedFiles, project);
    unsavedFiles.createOrUpdate({unsavedDataFileContainer(TESTDATA_DIR"/complete_extractor_function.cpp",
                                 TESTDATA_DIR"/complete_extractor_function_unsaved.cpp")});
    ClangCodeCompleteResults completeResults(getResults(translationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Method2"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, ChangeUnsavedFile)
{
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::ProjectPart project(Utf8StringLiteral("/path/to/projectfile"));
    TranslationUnit translationUnit(Utf8String::fromUtf8(TESTDATA_DIR"/complete_extractor_function.cpp"), unsavedFiles, project);
    unsavedFiles.createOrUpdate({unsavedDataFileContainer(TESTDATA_DIR"/complete_extractor_function.cpp",
                                 TESTDATA_DIR"/complete_extractor_function_unsaved.cpp")});
    ClangCodeCompleteResults completeResults(getResults(translationUnit, 20));
    unsavedFiles.createOrUpdate({unsavedDataFileContainer(TESTDATA_DIR"/complete_extractor_function.cpp",
                                 TESTDATA_DIR"/complete_extractor_function_unsaved_2.cpp")});
    completeResults = getResults(translationUnit, 20);

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Method3"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, ArgumentDefinition)
{
    variableTranslationUnit.cxTranslationUnit();
    project.setArguments({Utf8StringLiteral("-DArgumentDefinition")});
    ClangCodeCompleteResults completeResults(getResults(variableTranslationUnit, 35));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("ArgumentDefinitionVariable"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractor, NoArgumentDefinition)
{
    variableTranslationUnit.cxTranslationUnit();
    project.setArguments(Utf8StringVector());
    ClangCodeCompleteResults completeResults(getResults(variableTranslationUnit, 35));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, Not(HasCompletion(Utf8StringLiteral("ArgumentDefinitionVariable"),
                                             CodeCompletion::VariableCompletionKind,
                                             CodeCompletion::Available)));
}

TEST_F(CodeCompletionsExtractor, CompletionChunksFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Function"),
                                               CodeCompletionChunks({{CodeCompletionChunk::ResultType, Utf8StringLiteral("void")},
                                                                     {CodeCompletionChunk::TypedText, Utf8StringLiteral("Function")},
                                                                     {CodeCompletionChunk::LeftParen, Utf8StringLiteral("(")},
                                                                     {CodeCompletionChunk::RightParen, Utf8StringLiteral(")")}})));
}

TEST_F(CodeCompletionsExtractor, CompletionChunksFunctionWithOptionalChunks)
{
    ClangCodeCompleteResults completeResults(getResults(functionTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("FunctionWithOptional"),
                                               CodeCompletionChunks({{CodeCompletionChunk::ResultType, Utf8StringLiteral("void")},
                                                                     {CodeCompletionChunk::TypedText, Utf8StringLiteral("FunctionWithOptional")},
                                                                     {CodeCompletionChunk::LeftParen, Utf8StringLiteral("(")},
                                                                     {CodeCompletionChunk::Placeholder, Utf8StringLiteral("int x")},
                                                                     {CodeCompletionChunk::Optional, Utf8String(), CodeCompletionChunks({
                                                                          {CodeCompletionChunk::Comma, Utf8StringLiteral(", ")},
                                                                          {CodeCompletionChunk::Placeholder, Utf8StringLiteral("char y")},
                                                                          {CodeCompletionChunk::Comma, Utf8StringLiteral(", ")},
                                                                          {CodeCompletionChunk::Placeholder, Utf8StringLiteral("int z")}})},
                                                                     {CodeCompletionChunk::RightParen, Utf8StringLiteral(")")}})));
}

TEST_F(CodeCompletionsExtractor, CompletionChunksField)
{
    ClangCodeCompleteResults completeResults(getResults(variableTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Field"),
                                               CodeCompletionChunks({{CodeCompletionChunk::ResultType, Utf8StringLiteral("int")},
                                                                     {CodeCompletionChunk::TypedText, Utf8StringLiteral("Field")}})));
}

TEST_F(CodeCompletionsExtractor, CompletionChunksEnumerator)
{
    ClangCodeCompleteResults completeResults(getResults(enumerationTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Enumerator"),
                                               CodeCompletionChunks({{CodeCompletionChunk::ResultType, Utf8StringLiteral("Enumeration")},
                                                                     {CodeCompletionChunk::TypedText, Utf8StringLiteral("Enumerator")}})));
}

TEST_F(CodeCompletionsExtractor, CompletionChunksEnumeration)
{
    ClangCodeCompleteResults completeResults(getResults(enumerationTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Enumeration"),
                                               CodeCompletionChunks({{CodeCompletionChunk::TypedText, Utf8StringLiteral("Enumeration")}})));
}

TEST_F(CodeCompletionsExtractor, CompletionChunksClass)
{
    ClangCodeCompleteResults completeResults(getResults(classTranslationUnit, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Class"),
                                               CodeCompletionChunks({{CodeCompletionChunk::TypedText, Utf8StringLiteral("Class")}})));
}


}
