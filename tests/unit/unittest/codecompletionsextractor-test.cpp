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

#include <clangcodecompleteresults.h>
#include <clangdocument.h>
#include <clangfilepath.h>
#include <codecompletionsextractor.h>
#include <filecontainer.h>
#include <projectpart.h>
#include <projects.h>
#include <clangunsavedfilesshallowarguments.h>
#include <clangtranslationunit.h>
#include <clangdocuments.h>
#include <unsavedfiles.h>
#include <utf8stringvector.h>

#include <clang-c/Index.h>

#include <QFile>

using ClangBackEnd::CodeCompletionsExtractor;
using ClangBackEnd::ClangCodeCompleteResults;
using ClangBackEnd::FilePath;
using ClangBackEnd::Document;
using ClangBackEnd::CodeCompletion;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::UnsavedFilesShallowArguments;
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

MATCHER_P2(HasBriefComment, name, briefComment,
           std::string(negation ? "hasn't" : "has") + " completion of name " + PrintToString(name) +
           " with the brief comment " + PrintToString(briefComment))
{
    ::CodeCompletionsExtractor &extractor = const_cast<::CodeCompletionsExtractor&>(arg);
    while (extractor.next()) {
        if (extractor.currentCodeCompletion().text() == name) {
            if (extractor.currentCodeCompletion().briefComment() == briefComment) {
                return true;
            } else if (!extractor.peek(name)) {
                *result_listener << "briefComment is " << PrintToString(arg.currentCodeCompletion().briefComment()) << " and not " << PrintToString(briefComment);
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

class CodeCompletionsExtractor : public ::testing::Test
{
protected:
    ClangCodeCompleteResults getResults(const Document &document,
                                        uint line,
                                        uint column = 1,
                                        bool needsReparse = false);

protected:
    ClangBackEnd::ProjectPart project{Utf8StringLiteral("/path/to/projectfile")};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Document functionDocument{Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_function.cpp"), project, Utf8StringVector(), documents};
    Document variableDocument{Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_variable.cpp"), project, Utf8StringVector(), documents};
    Document classDocument{Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_class.cpp"), project, Utf8StringVector(), documents};
    Document namespaceDocument{Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_namespace.cpp"), project, Utf8StringVector(), documents};
    Document enumerationDocument{Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_enumeration.cpp"), project, Utf8StringVector(), documents};
    Document constructorDocument{Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_constructor.cpp"), project, Utf8StringVector(), documents};
    Document briefCommentDocument{Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_brief_comment.cpp"), project, Utf8StringVector(), documents};
};

using CodeCompletionsExtractorSlowTest = CodeCompletionsExtractor;

TEST_F(CodeCompletionsExtractorSlowTest, Function)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Function"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, TemplateFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("TemplateFunction"),
                                         CodeCompletion::TemplateFunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, Variable)
{
    ClangCodeCompleteResults completeResults(getResults(variableDocument, 4));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Var"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(NonTypeTemplateParameter))
{
    ClangCodeCompleteResults completeResults(getResults(variableDocument, 25, 19));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("NonTypeTemplateParameter"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}


TEST_F(CodeCompletionsExtractorSlowTest, VariableReference)
{
    ClangCodeCompleteResults completeResults(getResults(variableDocument, 12));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Var"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, Parameter)
{
    ClangCodeCompleteResults completeResults(getResults(variableDocument, 4));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Parameter"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, Field)
{
    ClangCodeCompleteResults completeResults(getResults(variableDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Field"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(Class))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Class"),
                                         CodeCompletion::ClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(Struct))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Struct"),
                                         CodeCompletion::ClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(Union))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Union"),
                                         CodeCompletion::ClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(Typedef))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("TypeDef"),
                                         CodeCompletion::TypeAliasCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(UsingAsTypeAlias))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("UsingClass"),
                                         CodeCompletion::TypeAliasCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(TemplateTypeParameter))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("TemplateTypeParameter"),
                                         CodeCompletion::ClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(TemplateClass))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("TemplateClass"),
                                         CodeCompletion::TemplateClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(TemplateTemplateParameter))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("TemplateTemplateParameter"),
                                         CodeCompletion::TemplateClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(ClassTemplatePartialSpecialization))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("ClassTemplatePartialSpecialization"),
                                         CodeCompletion::TemplateClassCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, Namespace)
{
    ClangCodeCompleteResults completeResults(getResults(namespaceDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Namespace"),
                                         CodeCompletion::NamespaceCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, NamespaceAlias)
{
    ClangCodeCompleteResults completeResults(getResults(namespaceDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("NamespaceAlias"),
                                         CodeCompletion::NamespaceCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, Enumeration)
{
    ClangCodeCompleteResults completeResults(getResults(enumerationDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Enumeration"),
                                         CodeCompletion::EnumerationCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, Enumerator)
{
    ClangCodeCompleteResults completeResults(getResults(enumerationDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Enumerator"),
                                         CodeCompletion::EnumeratorCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_Constructor)
{
    ClangCodeCompleteResults completeResults(getResults(constructorDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Constructor"),
                                         CodeCompletion::ConstructorCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, Destructor)
{
    ClangCodeCompleteResults completeResults(getResults(constructorDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("~Constructor"),
                                         CodeCompletion::DestructorCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, Method)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Method"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
    ASSERT_FALSE(extractor.currentCodeCompletion().hasParameters());
}

TEST_F(CodeCompletionsExtractorSlowTest, MethodWithParameters)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("MethodWithParameters"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
    ASSERT_TRUE(extractor.currentCodeCompletion().hasParameters());
}

TEST_F(CodeCompletionsExtractorSlowTest, Slot)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Slot"),
                                         CodeCompletion::SlotCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, Signal)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Signal"),
                                         CodeCompletion::SignalCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, MacroDefinition)
{
    ClangCodeCompleteResults completeResults(getResults(variableDocument, 35));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("MacroDefinition"),
                                         CodeCompletion::PreProcessorCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, FunctionMacro)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("FunctionMacro"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, IntKeyword)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("int"),
                                         CodeCompletion::KeywordCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, SwitchKeyword)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("switch"),
                                         CodeCompletion::KeywordCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, ClassKeyword)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("class"),
                                         CodeCompletion::KeywordCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, DeprecatedFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("DeprecatedFunction"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Deprecated));
}

TEST_F(CodeCompletionsExtractorSlowTest, NotAccessibleFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("NotAccessibleFunction"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::NotAccessible));
}

TEST_F(CodeCompletionsExtractorSlowTest, NotAvailableFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("NotAvailableFunction"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::NotAvailable));
}

TEST_F(CodeCompletionsExtractorSlowTest, UnsavedFile)
{
    Document document(Utf8String::fromUtf8(TESTDATA_DIR"/complete_extractor_function.cpp"), project, Utf8StringVector(), documents);
    unsavedFiles.createOrUpdate({unsavedDataFileContainer(TESTDATA_DIR"/complete_extractor_function.cpp",
                                 TESTDATA_DIR"/complete_extractor_function_unsaved.cpp")});
    ClangCodeCompleteResults completeResults(getResults(document, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Method2"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, ChangeUnsavedFile)
{
    Document document(Utf8String::fromUtf8(TESTDATA_DIR"/complete_extractor_function.cpp"), project, Utf8StringVector(), documents);
    unsavedFiles.createOrUpdate({unsavedDataFileContainer(TESTDATA_DIR"/complete_extractor_function.cpp",
                                 TESTDATA_DIR"/complete_extractor_function_unsaved.cpp")});
    ClangCodeCompleteResults completeResults(getResults(document, 20));
    unsavedFiles.createOrUpdate({unsavedDataFileContainer(TESTDATA_DIR"/complete_extractor_function.cpp",
                                 TESTDATA_DIR"/complete_extractor_function_unsaved_2.cpp")});
    completeResults = getResults(document, 20);

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("Method3"),
                                         CodeCompletion::FunctionCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, ArgumentDefinition)
{
    project.setArguments({Utf8StringLiteral("-DArgumentDefinition"), Utf8StringLiteral("-std=gnu++14")});
    ClangCodeCompleteResults completeResults(getResults(variableDocument, 35));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletion(Utf8StringLiteral("ArgumentDefinitionVariable"),
                                         CodeCompletion::VariableCompletionKind,
                                         CodeCompletion::Available));
}

TEST_F(CodeCompletionsExtractorSlowTest, NoArgumentDefinition)
{
    project.setArguments({Utf8StringLiteral("-std=gnu++14")});
    ClangCodeCompleteResults completeResults(getResults(variableDocument, 35));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, Not(HasCompletion(Utf8StringLiteral("ArgumentDefinitionVariable"),
                                             CodeCompletion::VariableCompletionKind,
                                             CodeCompletion::Available)));
}

TEST_F(CodeCompletionsExtractorSlowTest, CompletionChunksFunction)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Function"),
                                               CodeCompletionChunks({{CodeCompletionChunk::ResultType, Utf8StringLiteral("void")},
                                                                     {CodeCompletionChunk::TypedText, Utf8StringLiteral("Function")},
                                                                     {CodeCompletionChunk::LeftParen, Utf8StringLiteral("(")},
                                                                     {CodeCompletionChunk::RightParen, Utf8StringLiteral(")")}})));
}

TEST_F(CodeCompletionsExtractorSlowTest, CompletionChunksFunctionWithOptionalChunks)
{
    ClangCodeCompleteResults completeResults(getResults(functionDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("FunctionWithOptional"),
                                               CodeCompletionChunks({{CodeCompletionChunk::ResultType, Utf8StringLiteral("void")},
                                                                     {CodeCompletionChunk::TypedText, Utf8StringLiteral("FunctionWithOptional")},
                                                                     {CodeCompletionChunk::LeftParen, Utf8StringLiteral("(")},
                                                                     {CodeCompletionChunk::Placeholder, Utf8StringLiteral("int x")},
                                                                     {CodeCompletionChunk::Comma, Utf8StringLiteral(", "), true},
                                                                     {CodeCompletionChunk::Placeholder, Utf8StringLiteral("char y"), true},
                                                                     {CodeCompletionChunk::Comma, Utf8StringLiteral(", "), true},
                                                                     {CodeCompletionChunk::Placeholder, Utf8StringLiteral("int z"), true},
                                                                     {CodeCompletionChunk::RightParen, Utf8StringLiteral(")")}})));
}

TEST_F(CodeCompletionsExtractorSlowTest, CompletionChunksField)
{
    ClangCodeCompleteResults completeResults(getResults(variableDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Field"),
                                               CodeCompletionChunks({{CodeCompletionChunk::ResultType, Utf8StringLiteral("int")},
                                                                     {CodeCompletionChunk::TypedText, Utf8StringLiteral("Field")}})));
}

TEST_F(CodeCompletionsExtractorSlowTest, CompletionChunksEnumerator)
{
    ClangCodeCompleteResults completeResults(getResults(enumerationDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Enumerator"),
                                               CodeCompletionChunks({{CodeCompletionChunk::ResultType, Utf8StringLiteral("Enumeration")},
                                                                     {CodeCompletionChunk::TypedText, Utf8StringLiteral("Enumerator")}})));
}

TEST_F(CodeCompletionsExtractorSlowTest, CompletionChunksEnumeration)
{
    ClangCodeCompleteResults completeResults(getResults(enumerationDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Enumeration"),
                                               CodeCompletionChunks({{CodeCompletionChunk::TypedText, Utf8StringLiteral("Enumeration")}})));
}

TEST_F(CodeCompletionsExtractorSlowTest, DISABLED_ON_WINDOWS(CompletionChunksClass))
{
    ClangCodeCompleteResults completeResults(getResults(classDocument, 20));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasCompletionChunks(Utf8StringLiteral("Class"),
                                               CodeCompletionChunks({{CodeCompletionChunk::TypedText, Utf8StringLiteral("Class")}})));
}

TEST_F(CodeCompletionsExtractorSlowTest, BriefComment)
{
    ClangCodeCompleteResults completeResults(getResults(briefCommentDocument, 10, 1,
                                                        /*needsReparse=*/ true));

    ::CodeCompletionsExtractor extractor(completeResults.data());

    ASSERT_THAT(extractor, HasBriefComment(Utf8StringLiteral("BriefComment"), Utf8StringLiteral("A brief comment")));
}

ClangCodeCompleteResults CodeCompletionsExtractor::getResults(const Document &document,
                                                              uint line,
                                                              uint column,
                                                              bool needsReparse)
{
    document.parse();
    if (needsReparse)
        document.reparse();

    const Utf8String nativeFilePath = FilePath::toNativeSeparators(document.filePath());
    UnsavedFilesShallowArguments unsaved = unsavedFiles.shallowArguments();

    return ClangCodeCompleteResults(clang_codeCompleteAt(document.translationUnit().cxTranslationUnit(),
                                                         nativeFilePath.constData(),
                                                         line,
                                                         column,
                                                         unsaved.data(),
                                                         unsaved.count(),
                                                         CXCodeComplete_IncludeMacros | CXCodeComplete_IncludeCodePatterns | CXCodeComplete_IncludeBriefComments));
}

}
