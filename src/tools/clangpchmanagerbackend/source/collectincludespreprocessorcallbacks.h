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

#include "stringcache.h"

#include <clang/Basic/SourceManager.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>

#include <utils/smallstringvector.h>

#include <algorithm>

namespace ClangBackEnd {

class CollectIncludesPreprocessorCallbacks final : public clang::PPCallbacks
{
public:
    CollectIncludesPreprocessorCallbacks(clang::HeaderSearch &headerSearch,
                                         std::vector<uint> &includeIds,
                                         StringCache<Utils::PathString> &filePathCache,
                                         const std::vector<uint> &excludedIncludeUID,
                                         std::vector<uint>  &alreadyIncludedFileUIDs)
        : m_headerSearch(headerSearch),
          m_includeIds(includeIds),
          m_filePathCache(filePathCache),
          m_excludedIncludeUID(excludedIncludeUID),
          m_alreadyIncludedFileUIDs(alreadyIncludedFileUIDs)
    {}

    void InclusionDirective(clang::SourceLocation /*hashLocation*/,
                            const clang::Token &/*includeToken*/,
                            llvm::StringRef fileName,
                            bool /*isAngled*/,
                            clang::CharSourceRange /*fileNameRange*/,
                            const clang::FileEntry *file,
                            llvm::StringRef searchPath,
                            llvm::StringRef /*relativePath*/,
                            const clang::Module * /*imported*/) override
    {
        if (file) {
            auto fileUID = file->getUID();
            if (isNotInExcludedIncludeUID(fileUID)) {
                flagIncludeAlreadyRead(file);

                auto notAlreadyIncluded = isNotAlreadyIncluded(fileUID);
                if (notAlreadyIncluded.first) {
                    m_alreadyIncludedFileUIDs.insert(notAlreadyIncluded.second, fileUID);
                    Utils::PathString filePath = filePathFromFile(file);
                    if (!filePath.isEmpty()) {
                        uint includeId = m_filePathCache.stringId(filePath);
                        m_includeIds.emplace_back(includeId);
                    }
                }
            }
        }
    }

    bool isNotInExcludedIncludeUID(uint uid) const
    {
        return !std::binary_search(m_excludedIncludeUID.begin(),
                                   m_excludedIncludeUID.end(),
                                   uid);
    }

    std::pair<bool, std::vector<uint>::iterator> isNotAlreadyIncluded(uint uid) const
    {
        auto range = std::equal_range(m_alreadyIncludedFileUIDs.begin(),
                                      m_alreadyIncludedFileUIDs.end(),
                                      uid);

        return {range.first == range.second, range.first};
    }

    void flagIncludeAlreadyRead(const clang::FileEntry *file)
    {
        auto &headerFileInfo = m_headerSearch.getFileInfo(file);

        headerFileInfo.isImport = true;
        ++headerFileInfo.NumIncludes;
    }

    static Utils::PathString fromNativePath(Utils::PathString &&filePath)
    {
#ifdef _WIN32
        if (filePath.startsWith("\\\\?\\"))
            filePath = Utils::PathString(filePath.mid(4));
        filePath.replace('\\', '/');
#endif
        return std::move(filePath);
    }

    static Utils::PathString filePathFromFile(const clang::FileEntry *file)
    {
        clang::StringRef realPath = file->tryGetRealPathName();
        if (!realPath.empty())
            return fromNativePath({realPath.data(), realPath.size()});

        return fromNativePath(file->getName());
    }

private:
    clang::HeaderSearch &m_headerSearch;
    std::vector<uint> &m_includeIds;
    StringCache<Utils::PathString> &m_filePathCache;
    const std::vector<uint> &m_excludedIncludeUID;
    std::vector<uint> &m_alreadyIncludedFileUIDs;
};

} // namespace ClangBackEnd
