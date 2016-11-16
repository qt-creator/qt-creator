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

#pragma once

#include <sourcelocationscontainer.h>
#include <sourcerangescontainer.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <clang/Basic/SourceManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <cctype>

namespace ClangBackEnd {

inline
llvm::SmallString<256> absolutePath(const char *path)
{
    llvm::SmallString<256> absolutePath(path);

    if (!llvm::sys::path::is_absolute(absolutePath))
        llvm::sys::fs::make_absolute(absolutePath);

    return absolutePath;
}

template <typename Container>
Utils::SmallString fromNativePath(Container container)
{
    Utils::SmallString path(container.data(), container.size());

#ifdef _WIN32
    std::replace(path.begin(), path.end(), '\\', '/');
#endif

    return path;
}

inline Utils::SmallString getSourceText(const clang::FullSourceLoc &startFullSourceLocation,
                                        uint startOffset,
                                        uint endOffset)
{
    auto startBuffer = startFullSourceLocation.getBufferData();
    const auto bufferSize = endOffset - startOffset;

    return Utils::SmallString(startBuffer.data() + startOffset, bufferSize + 1);
}

inline void makePrintable(Utils::SmallString &text)
{
    text.replace("\n", " ");
    text.replace("\t", " ");

    auto end = std::unique(text.begin(), text.end(), [](char l, char r){
        return std::isspace(l) && std::isspace(r) && l == r;
    });
    text.resize(std::distance(text.begin(), end));
}

inline void appendSourceRangeToSourceRangesContainer(
        const clang::SourceRange &sourceRange,
        ClangBackEnd::SourceRangesContainer &sourceRangesContainer,
        const clang::SourceManager &sourceManager)
{
    clang::FullSourceLoc startFullSourceLocation(sourceRange.getBegin(), sourceManager);
    clang::FullSourceLoc endFullSourceLocation(sourceRange.getEnd(), sourceManager);
    if (startFullSourceLocation.isFileID() && endFullSourceLocation.isFileID()) {
        const auto startDecomposedLoction = startFullSourceLocation.getDecomposedLoc();
        const auto endDecomposedLoction = endFullSourceLocation.getDecomposedLoc();
        const auto fileId = startDecomposedLoction.first;
        const auto startOffset = startDecomposedLoction.second;
        const auto endOffset = endDecomposedLoction.second;
        const auto fileEntry = sourceManager.getFileEntryForID(fileId);
        auto filePath = absolutePath(fileEntry->getName());
        const auto fileName = llvm::sys::path::filename(filePath);
        llvm::sys::path::remove_filename(filePath);
        Utils::SmallString content = getSourceText(startFullSourceLocation,
                                                   startOffset,
                                                   endOffset);
        makePrintable(content);

        sourceRangesContainer.insertFilePath(fileId.getHashValue(),
                                             fromNativePath(filePath),
                                             fromNativePath(fileName));
        sourceRangesContainer.insertSourceRange(fileId.getHashValue(),
                                                startFullSourceLocation.getSpellingLineNumber(),
                                                startFullSourceLocation.getSpellingColumnNumber(),
                                                startOffset,
                                                endFullSourceLocation.getSpellingLineNumber(),
                                                endFullSourceLocation.getSpellingColumnNumber(),
                                                endOffset,
                                                std::move(content));
    }
}

inline
void appendSourceRangesToSourceRangesContainer(
        ClangBackEnd::SourceRangesContainer &sourceRangesContainer,
        const std::vector<clang::SourceRange> &sourceRanges,
        const clang::SourceManager &sourceManager)
{
    sourceRangesContainer.reserve(sourceRanges.size());

    for (const auto &sourceRange : sourceRanges)
        appendSourceRangeToSourceRangesContainer(sourceRange, sourceRangesContainer, sourceManager);
}

inline
void appendSourceLocationsToSourceLocationsContainer(
        ClangBackEnd::SourceLocationsContainer &sourceLocationsContainer,
        const std::vector<clang::SourceLocation> &sourceLocations,
        const clang::SourceManager &sourceManager)
{
    sourceLocationsContainer.reserve(sourceLocations.size());

    for (const auto &sourceLocation : sourceLocations) {
        clang::FullSourceLoc fullSourceLocation(sourceLocation, sourceManager);
        const auto decomposedLoction = fullSourceLocation.getDecomposedLoc();
        const auto fileId = decomposedLoction.first;
        const auto offset = decomposedLoction.second;
        const auto fileEntry = sourceManager.getFileEntryForID(fileId);
        auto filePath = absolutePath(fileEntry->getName());
        const auto fileName = llvm::sys::path::filename(filePath);
        llvm::sys::path::remove_filename(filePath);

        sourceLocationsContainer.insertFilePath(fileId.getHashValue(),
                                                fromNativePath(filePath),
                                                fromNativePath(fileName));
        sourceLocationsContainer.insertSourceLocation(fileId.getHashValue(),
                                                      fullSourceLocation.getSpellingLineNumber(),
                                                      fullSourceLocation.getSpellingColumnNumber(),
                                                      offset);
    }
}



} // namespace ClangBackEnd
