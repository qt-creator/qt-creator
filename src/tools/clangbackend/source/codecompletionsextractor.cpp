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

#include "clangbackend_global.h"
#include "clangstring.h"
#include "clangtranslationunit.h"
#include "codecompletionchunkconverter.h"
#include "sourcelocation.h"
#include "sourcerange.h"
#include "unsavedfile.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>

namespace ClangBackEnd {

CodeCompletionsExtractor::CodeCompletionsExtractor(const UnsavedFile &unsavedFile,
                                                   CXCodeCompleteResults *cxCodeCompleteResults)
    : unsavedFile(unsavedFile)
    , cxCodeCompleteResults(cxCodeCompleteResults)
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
        extractRequiredFixIts();

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

CodeCompletions CodeCompletionsExtractor::extractAll(bool onlyFunctionOverloads)
{
    CodeCompletions codeCompletions;
    codeCompletions.reserve(int(cxCodeCompleteResults->NumResults));

    while (next())
        codeCompletions.append(currentCodeCompletion_);

    handleCompletions(codeCompletions, onlyFunctionOverloads);

    return codeCompletions;
}

static CodeCompletions filterFunctionOverloads(const CodeCompletions &completions)
{
    return ::Utils::filtered(completions, [](const CodeCompletion &completion) {
        return completion.completionKind == CodeCompletion::FunctionOverloadCompletionKind;
    });
}

static void adaptOverloadsPriorities(CodeCompletions &codeCompletions)
{
    std::map<Utf8String, std::vector<CodeCompletion *>> cachedOverloads;
    for (CodeCompletion &currentCompletion : codeCompletions) {
        if (currentCompletion.completionKind != CodeCompletion::ConstructorCompletionKind
                && currentCompletion.completionKind != CodeCompletion::FunctionCompletionKind
                && currentCompletion.completionKind
                != CodeCompletion::FunctionDefinitionCompletionKind) {
            continue;
        }

        auto found = cachedOverloads.find(currentCompletion.text);
        if (found == cachedOverloads.end()) {
            cachedOverloads[currentCompletion.text].push_back(&currentCompletion);
        } else {
            const quint32 oldPriority = found->second.front()->priority;
            if (currentCompletion.priority >= oldPriority) {
                currentCompletion.priority = oldPriority;
            } else {
                const quint32 newPriority = currentCompletion.priority;
                for (CodeCompletion *completion : found->second)
                    completion->priority = newPriority;
            }
            found->second.push_back(&currentCompletion);
        }
    }
}

static void sortCodeCompletions(CodeCompletions &codeCompletions)
{
    auto currentItemsCompare = [](const CodeCompletion &first,
                                  const CodeCompletion &second) {
        // Items without fix-its come first.
        if (first.requiredFixIts.empty() != second.requiredFixIts.empty())
            return first.requiredFixIts.empty() > second.requiredFixIts.empty();

        return std::tie(first.priority, first.text, first.completionKind)
                < std::tie(second.priority, second.text, second.completionKind);
    };

    // Keep the order for the items with the same priority and name.
    std::stable_sort(codeCompletions.begin(), codeCompletions.end(), currentItemsCompare);
}

void CodeCompletionsExtractor::handleCompletions(CodeCompletions &codeCompletions,
                                                 bool onlyFunctionOverloads)
{
    if (onlyFunctionOverloads) {
        const CodeCompletions overloadCompletions = filterFunctionOverloads(codeCompletions);

        // If filtered completions are empty the assumption we need function overloads is wrong
        // therefore we do not use filtered results in that case.
        if (!overloadCompletions.isEmpty())
            codeCompletions = overloadCompletions;
    }

    adaptOverloadsPriorities(codeCompletions);
    sortCodeCompletions(codeCompletions);
}

void CodeCompletionsExtractor::extractCompletionKind()
{
    switch (currentCxCodeCompleteResult.CursorKind) {
        case CXCursor_FunctionTemplate:
            currentCodeCompletion_.completionKind = CodeCompletion::TemplateFunctionCompletionKind;
            break;
        case CXCursor_CXXMethod:
            extractMethodCompletionKind();
            break;
        case CXCursor_FunctionDecl:
        case CXCursor_ConversionFunction:
            currentCodeCompletion_.completionKind = CodeCompletion::FunctionCompletionKind;
            break;
        case CXCursor_VariableRef:
        case CXCursor_MemberRef:
        case CXCursor_VarDecl:
        case CXCursor_FieldDecl:
        case CXCursor_ParmDecl:
        case CXCursor_NonTypeTemplateParameter:
            currentCodeCompletion_.completionKind = CodeCompletion::VariableCompletionKind;
            break;
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
        case CXCursor_ClassDecl:
        case CXCursor_TemplateTypeParameter:
            currentCodeCompletion_.completionKind = CodeCompletion::ClassCompletionKind;
            break;
        case CXCursor_TypedefDecl:
        case CXCursor_TypeAliasDecl:
            currentCodeCompletion_.completionKind = CodeCompletion::TypeAliasCompletionKind;
            break;
        case CXCursor_ClassTemplatePartialSpecialization:
        case CXCursor_ClassTemplate:
        case CXCursor_TemplateTemplateParameter:
            currentCodeCompletion_.completionKind = CodeCompletion::TemplateClassCompletionKind;
            break;
        case CXCursor_Namespace:
        case CXCursor_NamespaceAlias:
            currentCodeCompletion_.completionKind = CodeCompletion::NamespaceCompletionKind;
            break;
        case CXCursor_EnumDecl:
            currentCodeCompletion_.completionKind = CodeCompletion::EnumerationCompletionKind;
            break;
        case CXCursor_EnumConstantDecl:
            currentCodeCompletion_.completionKind = CodeCompletion::EnumeratorCompletionKind;
            break;
        case CXCursor_Constructor:
            currentCodeCompletion_.completionKind = CodeCompletion::ConstructorCompletionKind;
            break;
        case CXCursor_Destructor:
            currentCodeCompletion_.completionKind = CodeCompletion::DestructorCompletionKind;
            break;
        case CXCursor_MacroDefinition:
            extractMacroCompletionKind();
            break;
        case CXCursor_NotImplemented:
            currentCodeCompletion_.completionKind = CodeCompletion::KeywordCompletionKind;
            break;
        case CXCursor_OverloadCandidate:
            currentCodeCompletion_.completionKind = CodeCompletion::FunctionOverloadCompletionKind;
            break;
        default:
            currentCodeCompletion_.completionKind = CodeCompletion::Other;
    }
}

void CodeCompletionsExtractor::extractText()
{
    const uint completionChunkCount = clang_getNumCompletionChunks(currentCxCodeCompleteResult.CompletionString);
    for (uint chunkIndex = 0; chunkIndex < completionChunkCount; ++chunkIndex) {
        const CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(currentCxCodeCompleteResult.CompletionString, chunkIndex);
        if (chunkKind == CXCompletionChunk_TypedText) {
            currentCodeCompletion_.text = CodeCompletionChunkConverter::chunkText(currentCxCodeCompleteResult.CompletionString, chunkIndex);
            break;
        }
    }
}

void CodeCompletionsExtractor::extractMethodCompletionKind()
{
    CXCompletionString cxCompletionString = cxCodeCompleteResults->Results[cxCodeCompleteResultIndex].CompletionString;

    const unsigned long long contexts = clang_codeCompleteGetContexts(cxCodeCompleteResults);

    const uint annotationCount = clang_getCompletionNumAnnotations(cxCompletionString);

    for (uint annotationIndex = 0; annotationIndex < annotationCount; ++annotationIndex) {
        ClangString annotation = clang_getCompletionAnnotation(cxCompletionString, annotationIndex);

        if (annotation == Utf8StringLiteral("qt_signal")) {
            currentCodeCompletion_.completionKind = CodeCompletion::SignalCompletionKind;
            return;
        }

        if (annotation == Utf8StringLiteral("qt_slot")) {
            currentCodeCompletion_.completionKind = CodeCompletion::SlotCompletionKind;
            return;
        }
    }

    currentCodeCompletion_.completionKind = CodeCompletion::FunctionDefinitionCompletionKind;
    if ((contexts & CXCompletionContext_DotMemberAccess)
            || (contexts & CXCompletionContext_ArrowMemberAccess)) {
        currentCodeCompletion_.completionKind = CodeCompletion::FunctionCompletionKind;
    }
}

void CodeCompletionsExtractor::extractMacroCompletionKind()
{
    CXCompletionString cxCompletionString = cxCodeCompleteResults->Results[cxCodeCompleteResultIndex].CompletionString;

    const uint completionChunkCount = clang_getNumCompletionChunks(cxCompletionString);

    for (uint chunkIndex = 0; chunkIndex < completionChunkCount; ++chunkIndex) {
        CXCompletionChunkKind kind = clang_getCompletionChunkKind(cxCompletionString, chunkIndex);
        if (kind == CXCompletionChunk_Placeholder) {
            currentCodeCompletion_.completionKind = CodeCompletion::FunctionCompletionKind;
            return;
        }
    }

    currentCodeCompletion_.completionKind = CodeCompletion::PreProcessorCompletionKind;
}

void CodeCompletionsExtractor::extractPriority()
{
    CXCompletionString cxCompletionString = cxCodeCompleteResults->Results[cxCodeCompleteResultIndex].CompletionString;
    quint32 priority = clang_getCompletionPriority(cxCompletionString);
    currentCodeCompletion_.priority = priority;
}

void CodeCompletionsExtractor::extractAvailability()
{
    CXCompletionString cxCompletionString = cxCodeCompleteResults->Results[cxCodeCompleteResultIndex].CompletionString;
    CXAvailabilityKind cxAvailabilityKind = clang_getCompletionAvailability(cxCompletionString);

    switch (cxAvailabilityKind) {
        case CXAvailability_Available:
            currentCodeCompletion_.availability = CodeCompletion::Available;
            break;
        case CXAvailability_Deprecated:
            currentCodeCompletion_.availability = CodeCompletion::Deprecated;
            break;
        case CXAvailability_NotAvailable:
            currentCodeCompletion_.availability = CodeCompletion::NotAvailable;
            break;
        case CXAvailability_NotAccessible:
            currentCodeCompletion_.availability = CodeCompletion::NotAccessible;
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
            currentCodeCompletion_.hasParameters = nextChunkKind != CXCompletionChunk_RightParen;
            return;
        }
    }
}

void CodeCompletionsExtractor::extractBriefComment()
{
    ClangString briefComment = clang_getCompletionBriefComment(currentCxCodeCompleteResult.CompletionString);

    currentCodeCompletion_.briefComment = briefComment;
}

void CodeCompletionsExtractor::extractCompletionChunks()
{
    currentCodeCompletion_.chunks = CodeCompletionChunkConverter::extract(currentCxCodeCompleteResult.CompletionString);
}

SourceRangeContainer toRangeContainer(const UnsavedFile &file, CXSourceRange cxSourceRange)
{
    const CXSourceLocation start = clang_getRangeStart(cxSourceRange);
    const CXSourceLocation end = clang_getRangeEnd(cxSourceRange);

    uint startLine = 0;
    uint startColumn = 0;
    uint endLine = 0;
    uint endColumn = 0;
    clang_getFileLocation(start, nullptr, &startLine, &startColumn, nullptr);
    clang_getFileLocation(end, nullptr, &endLine, &endColumn, nullptr);
    QTC_ASSERT(startLine == endLine, return SourceRangeContainer(););

    const Utf8String lineText = file.lineRange(startLine, endLine);
    startColumn = QString(lineText.mid(0, startColumn - 1)).size() + 1;
    endColumn = QString(lineText.mid(0, endColumn - 1)).size() + 1;

    return SourceRangeContainer(SourceLocationContainer(file.filePath(), startLine, startColumn),
                                SourceLocationContainer(file.filePath(), endLine, endColumn));
}

void CodeCompletionsExtractor::extractRequiredFixIts()
{
#ifdef IS_COMPLETION_FIXITS_BACKPORTED
    unsigned fixItsNumber = clang_getCompletionNumFixIts(cxCodeCompleteResults,
                                                         cxCodeCompleteResultIndex);

    if (!fixItsNumber)
        return;

    CXSourceRange range;
    for (unsigned i = 0; i < fixItsNumber; ++i) {
        ClangString fixIt = clang_getCompletionFixIt(cxCodeCompleteResults,
                                                     cxCodeCompleteResultIndex,
                                                     i,
                                                     &range);
        currentCodeCompletion_.requiredFixIts.push_back(
                    FixItContainer(Utf8String(fixIt), toRangeContainer(unsavedFile, range)));
    }
#endif
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
    if (currentCodeCompletion_.availability != CodeCompletion::Available)
        currentCodeCompletion_.priority = currentCodeCompletion_.priority * 100;
}

void CodeCompletionsExtractor::decreasePriorityForDestructors()
{
    if (currentCodeCompletion_.completionKind == CodeCompletion::DestructorCompletionKind)
        currentCodeCompletion_.priority = currentCodeCompletion_.priority * 100;
}

void CodeCompletionsExtractor::decreasePriorityForSignals()
{
    if (currentCodeCompletion_.completionKind == CodeCompletion::SignalCompletionKind)
        currentCodeCompletion_.priority = currentCodeCompletion_.priority * 100;
}

void CodeCompletionsExtractor::decreasePriorityForQObjectInternals()
{
    quint32 priority = currentCodeCompletion_.priority;

    if (currentCodeCompletion_.text.startsWith("qt_"))
        priority *= 100;

    if (currentCodeCompletion_.text == Utf8StringLiteral("metaObject"))
        priority *= 10;

    if (currentCodeCompletion_.text == Utf8StringLiteral("staticMetaObject"))
        priority *= 100;

    currentCodeCompletion_.priority = priority;
}

bool isOperator(CXCursorKind cxCursorKind, const Utf8String &name)
{
    return cxCursorKind == CXCursor_ConversionFunction
            || (cxCursorKind == CXCursor_CXXMethod
                && name.startsWith(Utf8StringLiteral("operator")));
}

void CodeCompletionsExtractor::decreasePriorityForOperators()
{
    quint32 priority = currentCodeCompletion_.priority;

    if (isOperator(currentCxCodeCompleteResult.CursorKind, currentCodeCompletion().text))
        priority *= 100;

    currentCodeCompletion_.priority = priority;
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
    os << "name: " << extractor.currentCodeCompletion().text
       << ", kind: " <<  extractor.currentCodeCompletion().completionKind
       << ", priority: " <<  extractor.currentCodeCompletion().priority
       << ", kind: " <<  extractor.currentCodeCompletion().availability;

    return os;
}

} // namespace ClangBackEnd

