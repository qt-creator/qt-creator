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

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <clang/Basic/SourceManager.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <utils/smallstringvector.h>

#include <algorithm>

#include <QDebug>

namespace ClangBackEnd {

class CollectIncludesPreprocessorCallbacks final : public clang::PPCallbacks
{
public:
    CollectIncludesPreprocessorCallbacks(clang::HeaderSearch &headerSearch,
                                         Utils::SmallStringVector &includes,
                                         const std::vector<uint> &excludedIncludeUID,
                                         std::vector<uint>  &alreadyIncludedFileUIDs)
        : headerSearch(headerSearch),
          includes(includes),
          excludedIncludeUID(excludedIncludeUID),
          alreadyIncludedFileUIDs(alreadyIncludedFileUIDs)
    {}

    void InclusionDirective(clang::SourceLocation /*hashLocation*/,
                            const clang::Token &/*includeToken*/,
                            llvm::StringRef fileName,
                            bool /*isAngled*/,
                            clang::CharSourceRange /*fileNameRange*/,
                            const clang::FileEntry *file,
                            llvm::StringRef /*searchPath*/,
                            llvm::StringRef /*relativePath*/,
                            const clang::Module */*imported*/) override
    {
        auto fileUID = file->getUID();

        flagIncludeAlreadyRead(file);

        if (isNotInExcludedIncludeUID(fileUID)) {
            auto notAlreadyIncluded = isNotAlreadyIncluded(fileUID);
            if (notAlreadyIncluded.first) {
                alreadyIncludedFileUIDs.insert(notAlreadyIncluded.second, file->getUID());
                includes.emplace_back(fileName.data(), fileName.size());
            }
        }
    }

    bool isNotInExcludedIncludeUID(uint uid) const
    {
        return !std::binary_search(excludedIncludeUID.begin(),
                                   excludedIncludeUID.end(),
                                   uid);
    }


    std::pair<bool, std::vector<uint>::iterator> isNotAlreadyIncluded(uint uid)
    {
        auto range = std::equal_range(alreadyIncludedFileUIDs.begin(),
                                      alreadyIncludedFileUIDs.end(),
                                      uid);

        return {range.first == range.second, range.first};
    }

    void flagIncludeAlreadyRead(const clang::FileEntry *file)
    {
        auto &headerFileInfo = headerSearch.getFileInfo(file);

        headerFileInfo.isImport = true;
        ++headerFileInfo.NumIncludes;

    }

private:
    clang::HeaderSearch &headerSearch;
    std::vector<Utils::SmallString>  &includes;
    const std::vector<uint> &excludedIncludeUID;
    std::vector<uint>  &alreadyIncludedFileUIDs;
};

} // namespace ClangBackEnd
