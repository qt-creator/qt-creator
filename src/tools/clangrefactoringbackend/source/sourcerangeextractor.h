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

#include <utils/smallstringfwd.h>

#include <vector>

using uint = unsigned int;

namespace llvm {
class StringRef;
}

namespace clang {
class SourceManager;
class LangOptions;
class SourceRange;
class FullSourceLoc;
}

namespace ClangBackEnd {

class SourceRangesContainer;
class SourceRangeWithTextContainer;

class SourceRangeExtractor
{
public:
    SourceRangeExtractor(const clang::SourceManager &sourceManager,
                         const clang::LangOptions &languageOptions,
                         SourceRangesContainer &sourceRangesContainer);

    void addSourceRange(const clang::SourceRange &sourceRange);
    void addSourceRanges(const std::vector<clang::SourceRange> &sourceRanges);

    const std::vector<SourceRangeWithTextContainer> &sourceRangeWithTextContainers() const;

    static const char *findStartOfLineInBuffer(const llvm::StringRef buffer, uint startOffset);
    static const char *findEndOfLineInBuffer(llvm::StringRef buffer, uint endOffset);
    static Utils::SmallString getExpandedText(llvm::StringRef buffer, uint startOffset, uint endOffset);

    const clang::SourceRange extendSourceRangeToLastTokenEnd(const clang::SourceRange sourceRange);

private:
    void insertSourceRange(uint fileHash,
                           Utils::SmallString &&directoryPath,
                           Utils::SmallString &&fileName,
                           const clang::FullSourceLoc &startLocation,
                           uint startOffset,
                           const clang::FullSourceLoc &endLocation,
                           uint endOffset,
                           Utils::SmallString &&lineSnippet);

private:
    const clang::SourceManager &sourceManager;
    const clang::LangOptions &languageOptions;
    SourceRangesContainer &sourceRangesContainer;
};

} // namespace ClangBackEnd
