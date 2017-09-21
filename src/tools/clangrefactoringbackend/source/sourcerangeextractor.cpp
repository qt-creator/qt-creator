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

#include "sourcerangeextractor.h"

#include "sourcelocationsutils.h"

#include <sourcerangescontainer.h>
#include <filepathcachinginterface.h>

#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/ADT/SmallVector.h>

namespace ClangBackEnd {

SourceRangeExtractor::SourceRangeExtractor(
        const clang::SourceManager &sourceManager,
        const clang::LangOptions &languageOptions,
        FilePathCachingInterface &filePathCache,
        SourceRangesContainer &sourceRangesContainer)
    : sourceManager(sourceManager),
      languageOptions(languageOptions),
      filePathCache(filePathCache),
      sourceRangesContainer(sourceRangesContainer)
{
}

namespace {
template<typename Iterator>
std::reverse_iterator<Iterator> make_reverse_iterator(Iterator iterator)
{
    return std::reverse_iterator<Iterator>(iterator);
}

}

const char *SourceRangeExtractor::findStartOfLineInBuffer(llvm::StringRef buffer, uint startOffset)
{
    auto beginText = buffer.begin() + startOffset;
    auto reverseBegin = make_reverse_iterator(beginText);
    auto reverseEnd = make_reverse_iterator(buffer.begin());

    auto found = std::find_if(reverseBegin,
                              reverseEnd,
                              [] (const char character) {
        return character == '\n' || character == '\r';
    });

    if (found != reverseEnd)
        return found.base();

    return buffer.begin();
}

const char *SourceRangeExtractor::findEndOfLineInBuffer(llvm::StringRef buffer, uint endOffset)
{
    auto beginText = buffer.begin() + endOffset;

    auto found = std::find_if(beginText,
                              buffer.end(),
                              [] (const char character) {
        return character == '\n' || character == '\r';
    });

    if (found != buffer.end())
        return found;

    return buffer.end();
}

Utils::SmallString SourceRangeExtractor::getExpandedText(llvm::StringRef buffer,
                                                    uint startOffset,
                                                    uint endOffset)
{
    auto startBuffer = findStartOfLineInBuffer(buffer, startOffset);
    auto endBuffer = findEndOfLineInBuffer(buffer, endOffset);

    return Utils::SmallString(startBuffer, endBuffer);
}

const clang::SourceRange SourceRangeExtractor::extendSourceRangeToLastTokenEnd(const clang::SourceRange sourceRange)
{
    auto endLocation = sourceRange.getEnd();
    uint length = clang::Lexer::MeasureTokenLength(sourceManager.getSpellingLoc(endLocation),
                                                   sourceManager,
                                                   languageOptions);
    endLocation = endLocation.getLocWithOffset(length);

    return {sourceRange.getBegin(), endLocation};
}

void SourceRangeExtractor::insertSourceRange(FilePathId filePathId,
                                             const clang::FullSourceLoc &startLocation,
                                             uint startOffset,
                                             const clang::FullSourceLoc &endLocation,
                                             uint endOffset,
                                             Utils::SmallString &&lineSnippet)
{
    sourceRangesContainer.insertSourceRange(filePathId,
                                            startLocation.getSpellingLineNumber(),
                                            startLocation.getSpellingColumnNumber(),
                                            startOffset,
                                            endLocation.getSpellingLineNumber(),
                                            endLocation.getSpellingColumnNumber(),
                                            endOffset,
                                            std::move(lineSnippet));
}

FilePathId SourceRangeExtractor::findFileId(clang::FileID fileId, const clang::FileEntry *fileEntry) const
{
    auto found = m_fileIdMapping.find(fileId.getHashValue());
    if (found != m_fileIdMapping.end()) {
       return found->second;
    }

    auto filePath = absolutePath(fileEntry->getName());
    return filePathCache.filePathId(fromNativePath(filePath));
}

void SourceRangeExtractor::addSourceRange(const clang::SourceRange &sourceRange)
{
    auto extendedSourceRange = extendSourceRangeToLastTokenEnd(sourceRange);

    clang::FullSourceLoc startSourceLocation(extendedSourceRange.getBegin(), sourceManager);
    clang::FullSourceLoc endSourceLocation(extendedSourceRange.getEnd(), sourceManager);
    if (startSourceLocation.isFileID() && endSourceLocation.isFileID()) {
        const auto startDecomposedLoction = startSourceLocation.getDecomposedLoc();
        const auto endDecomposedLoction = endSourceLocation.getDecomposedLoc();
        const auto fileId = startDecomposedLoction.first;
        const auto startOffset = startDecomposedLoction.second;
        const auto endOffset = endDecomposedLoction.second;
        const auto fileEntry = sourceManager.getFileEntryForID(fileId);

        Utils::SmallString lineSnippet = getExpandedText(startSourceLocation.getBufferData(),
                                                         startOffset,
                                                         endOffset);

        insertSourceRange(findFileId(fileId, fileEntry),
                          startSourceLocation,
                          startOffset,
                          endSourceLocation,
                          endOffset,
                          std::move(lineSnippet));
    }
}

void SourceRangeExtractor::addSourceRanges(const std::vector<clang::SourceRange> &sourceRanges)
{
    sourceRangesContainer.reserve(sourceRanges.size() + sourceRangeWithTextContainers().size());

    for (const clang::SourceRange &sourceRange : sourceRanges)
        addSourceRange(sourceRange);
}

const std::vector<SourceRangeWithTextContainer> &SourceRangeExtractor::sourceRangeWithTextContainers() const
{
    return sourceRangesContainer.sourceRangeWithTextContainers();
}

} // namespace ClangBackEnd
