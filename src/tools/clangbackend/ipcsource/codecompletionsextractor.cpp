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

#include "codecompletionsextractor.h"

#include "clangstring.h"
#include "codecompletionchunkconverter.h"

#include <QDebug>

namespace ClangBackEnd {

CodeCompletionsExtractor::CodeCompletionsExtractor(CXCodeCompleteResults *cxCodeCompleteResults)
    : cxCodeCompleteResults(cxCodeCompleteResults)
{

}

bool CodeCompletionsExtractor::next()
{
    const uint cxCodeCompleteResultCount = cxCodeCompleteResults->NumResults;

    if (cxCodeCompleteResultIndex < cxCodeCompleteResultCount) {
        currentCxCodeCompleteResult = cxCodeCompleteResults->Results[cxCodeCompleteResultIndex];

        currentCodeCompletion_ = CodeCompletion();

        extractCompletionKind();
        extractText();
        extractPriority();
        extractAvailability();
        extractHasParameters();
        extractBriefComment();
        extractCompletionChunks();
        adaptPriority();

        ++cxCodeCompleteResultIndex;

        return true;
    }

    return false;
}

bool CodeCompletionsExtractor::peek(const Utf8String &name)
{
    const uint cxCodeCompleteResultCount = cxCodeCompleteResults->NumResults;

    uint peekCxCodeCompleteResultIndex = cxCodeCompleteResultIndex;

    while (peekCxCodeCompleteResultIndex < cxCodeCompleteResultCount) {
        if (hasText(name, cxCodeCompleteResults->Results[peekCxCodeCompleteResultIndex].CompletionString))
            return true;

        ++peekCxCodeCompleteResultIndex;
    }

    return false;
}

CodeCompletions CodeCompletionsExtractor::extractAll()
{
    CodeCompletions codeCompletions;
    codeCompletions.reserve(int(cxCodeCompleteResults->NumResults));

    while (next())
        codeCompletions.append(currentCodeCompletion_);

    return codeCompletions;
}

void CodeCompletionsExtractor::extractCompletionKind()
{
    switch (currentCxCodeCompleteResult.CursorKind) {
        case CXCursor_FunctionTemplate:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::TemplateFunctionCompletionKind);
            break;
        case CXCursor_CXXMethod:
            extractMethodCompletionKind();
            break;
        case CXCursor_FunctionDecl:
        case CXCursor_ConversionFunction:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::FunctionCompletionKind);
            break;
        case CXCursor_VariableRef:
        case CXCursor_VarDecl:
        case CXCursor_FieldDecl:
        case CXCursor_ParmDecl:
        case CXCursor_NonTypeTemplateParameter:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::VariableCompletionKind);
            break;
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
        case CXCursor_ClassDecl:
        case CXCursor_TemplateTypeParameter:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::ClassCompletionKind);
            break;
        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::TypeAliasCompletionKind);
            break;
        case CXCursor_ClassTemplatePartialSpecialization:
        case CXCursor_ClassTemplate:
        case CXCursor_TemplateTemplateParameter:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::TemplateClassCompletionKind);
            break;
        case CXCursor_Namespace:
        case CXCursor_NamespaceAlias:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::NamespaceCompletionKind);
            break;
        case CXCursor_EnumDecl:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::EnumerationCompletionKind);
            break;
        case CXCursor_EnumConstantDecl:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::EnumeratorCompletionKind);
            break;
        case CXCursor_Constructor:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::ConstructorCompletionKind);
            break;
        case CXCursor_Destructor:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::DestructorCompletionKind);
            break;
        case CXCursor_MacroDefinition:
            extractMacroCompletionKind();
            break;
        case CXCursor_NotImplemented:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::KeywordCompletionKind);
            break;
        case CXCursor_OverloadCandidate:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::FunctionOverloadCompletionKind);
            break;
        default:
            currentCodeCompletion_.setCompletionKind(CodeCompletion::Other);
    }
}

void CodeCompletionsExtractor::extractText()
{
    const uint completionChunkCount = clang_getNumCompletionChunks(currentCxCodeCompleteResult.CompletionString);
    for (uint chunkIndex = 0; chunkIndex < completionChunkCount; ++chunkIndex) {
        const CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(currentCxCodeCompleteResult.CompletionString, chunkIndex);
        if (chunkKind == CXCompletionChunk_TypedText) {
            currentCodeCompletion_.setText(CodeCompletionChunkConverter::chunkText(currentCxCodeCompleteResult.CompletionString, chunkIndex));
            break;
        }
    }
}

void CodeCompletionsExtractor::extractMethodCompletionKind()
{
    CXCompletionString cxCompletionString = cxCodeCompleteResults->Results[cxCodeCompleteResultIndex].CompletionString;
    const uint annotationCount = clang_getCompletionNumAnnotations(cxCompletionString);

    for (uint annotationIndex = 0; annotationIndex < annotationCount; ++annotationIndex) {
        ClangString annotation = clang_getCompletionAnnotation(cxCompletionString, annotationIndex);

        if (annotation == Utf8StringLiteral("qt_signal")) {
            currentCodeCompletion_.setCompletionKind(CodeCompletion::SignalCompletionKind);
            return;
        }

        if (annotation == Utf8StringLiteral("qt_slot")) {
            currentCodeCompletion_.setCompletionKind(CodeCompletion::SlotCompletionKind);
            return;
        }
    }

    currentCodeCompletion_.setCompletionKind(CodeCompletion::FunctionCompletionKind);
}

void CodeCompletionsExtractor::extractMacroCompletionKind()
{
    CXCompletionString cxCompletionString = cxCodeCompleteResults->Results[cxCodeCompleteResultIndex].CompletionString;

    const uint completionChunkCount = clang_getNumCompletionChunks(cxCompletionString);

    for (uint chunkIndex = 0; chunkIndex < completionChunkCount; ++chunkIndex) {
        CXCompletionChunkKind kind = clang_getCompletionChunkKind(cxCompletionString, chunkIndex);
        if (kind == CXCompletionChunk_Placeholder) {
            currentCodeCompletion_.setCompletionKind(CodeCompletion::FunctionCompletionKind);
            return;
        }
    }

    currentCodeCompletion_.setCompletionKind(CodeCompletion::PreProcessorCompletionKind);
}

void CodeCompletionsExtractor::extractPriority()
{
    CXCompletionString cxCompletionString = cxCodeCompleteResults->Results[cxCodeCompleteResultIndex].CompletionString;
    quint32 priority = clang_getCompletionPriority(cxCompletionString);
    currentCodeCompletion_.setPriority(priority);
}

void CodeCompletionsExtractor::extractAvailability()
{
    CXCompletionString cxCompletionString = cxCodeCompleteResults->Results[cxCodeCompleteResultIndex].CompletionString;
    CXAvailabilityKind cxAvailabilityKind = clang_getCompletionAvailability(cxCompletionString);

    switch (cxAvailabilityKind) {
        case CXAvailability_Available:
            currentCodeCompletion_.setAvailability(CodeCompletion::Available);
            break;
        case CXAvailability_Deprecated:
            currentCodeCompletion_.setAvailability(CodeCompletion::Deprecated);
            break;
        case CXAvailability_NotAvailable:
            currentCodeCompletion_.setAvailability(CodeCompletion::NotAvailable);
            break;
        case CXAvailability_NotAccessible:
            currentCodeCompletion_.setAvailability(CodeCompletion::NotAccessible);
            break;
    }
}

void CodeCompletionsExtractor::extractHasParameters()
{
    const uint completionChunkCount = clang_getNumCompletionChunks(currentCxCodeCompleteResult.CompletionString);
    for (uint chunkIndex = 0; chunkIndex < completionChunkCount; ++chunkIndex) {
        const CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(currentCxCodeCompleteResult.CompletionString, chunkIndex);
        if (chunkKind == CXCompletionChunk_LeftParen) {
            const CXCompletionChunkKind nextChunkKind = clang_getCompletionChunkKind(currentCxCodeCompleteResult.CompletionString, chunkIndex + 1);
            currentCodeCompletion_.setHasParameters(nextChunkKind != CXCompletionChunk_RightParen);
            return;
        }
    }
}

void CodeCompletionsExtractor::extractBriefComment()
{
    ClangString briefComment = clang_getCompletionBriefComment(currentCxCodeCompleteResult.CompletionString);

    currentCodeCompletion_.setBriefComment(briefComment);
}

void CodeCompletionsExtractor::extractCompletionChunks()
{
    currentCodeCompletion_.setChunks(CodeCompletionChunkConverter::extract(currentCxCodeCompleteResult.CompletionString));
}

void CodeCompletionsExtractor::adaptPriority()
{
    decreasePriorityForDestructors();
    decreasePriorityForNonAvailableCompletions();
    decreasePriorityForQObjectInternals();
    decreasePriorityForSignals();
    decreasePriorityForOperators();
}

void CodeCompletionsExtractor::decreasePriorityForNonAvailableCompletions()
{
    if (currentCodeCompletion_.availability() != CodeCompletion::Available)
        currentCodeCompletion_.setPriority(currentCodeCompletion_.priority() * 100);
}

void CodeCompletionsExtractor::decreasePriorityForDestructors()
{
    if (currentCodeCompletion_.completionKind() == CodeCompletion::DestructorCompletionKind)
        currentCodeCompletion_.setPriority(currentCodeCompletion_.priority() * 100);
}

void CodeCompletionsExtractor::decreasePriorityForSignals()
{
    if (currentCodeCompletion_.completionKind() == CodeCompletion::SignalCompletionKind)
        currentCodeCompletion_.setPriority(currentCodeCompletion_.priority() * 100);
}

void CodeCompletionsExtractor::decreasePriorityForQObjectInternals()
{
    quint32 priority = currentCodeCompletion_.priority();

    if (currentCodeCompletion_.text().startsWith("qt_"))
        priority *= 100;

    if (currentCodeCompletion_.text() == Utf8StringLiteral("metaObject"))
        priority *= 10;

    if (currentCodeCompletion_.text() == Utf8StringLiteral("staticMetaObject"))
        priority *= 100;

    currentCodeCompletion_.setPriority(priority);
}

bool isOperator(CXCursorKind cxCursorKind, const Utf8String &name)
{
    return cxCursorKind == CXCursor_ConversionFunction
            || (cxCursorKind == CXCursor_CXXMethod
                && name.startsWith(Utf8StringLiteral("operator")));
}

void CodeCompletionsExtractor::decreasePriorityForOperators()
{
    quint32 priority = currentCodeCompletion_.priority();

    if (isOperator(currentCxCodeCompleteResult.CursorKind, currentCodeCompletion().text()))
        priority *= 100;

    currentCodeCompletion_.setPriority(priority);
}

bool CodeCompletionsExtractor::hasText(const Utf8String &text, CXCompletionString cxCompletionString) const
{
    const uint completionChunkCount = clang_getNumCompletionChunks(cxCompletionString);

    for (uint chunkIndex = 0; chunkIndex < completionChunkCount; ++chunkIndex) {
        const CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(cxCompletionString, chunkIndex);
        if (chunkKind == CXCompletionChunk_TypedText) {
            const ClangString currentText(clang_getCompletionChunkText(cxCompletionString, chunkIndex));
            return text == currentText;
        }
    }

    return false;
}

const CodeCompletion &CodeCompletionsExtractor::currentCodeCompletion() const
{
    return currentCodeCompletion_;
}

std::ostream &operator<<(std::ostream &os, const CodeCompletionsExtractor &extractor)
{
    os << "name: " << extractor.currentCodeCompletion().text()
       << ", kind: " <<  extractor.currentCodeCompletion().completionKind()
       << ", priority: " <<  extractor.currentCodeCompletion().priority()
       << ", kind: " <<  extractor.currentCodeCompletion().availability();

    return os;
}

} // namespace ClangBackEnd

