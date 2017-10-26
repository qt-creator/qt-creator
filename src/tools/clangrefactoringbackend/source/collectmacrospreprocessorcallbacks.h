/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "sourcelocationsutils.h"

#include <filepath.h>
#include <filepathid.h>

#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>

namespace ClangBackEnd {

class CollectMacrosPreprocessorCallbacks final : public clang::PPCallbacks
{
public:
    CollectMacrosPreprocessorCallbacks(FilePathIds &sourceFiles,
                                       FilePathCachingInterface &filePathCache)
        : m_sourceFiles(sourceFiles),
          m_filePathCache(filePathCache)

    {
    }

    void InclusionDirective(clang::SourceLocation /*hashLocation*/,
                            const clang::Token &/*includeToken*/,
                            llvm::StringRef /*fileName*/,
                            bool /*isAngled*/,
                            clang::CharSourceRange /*fileNameRange*/,
                            const clang::FileEntry *file,
                            llvm::StringRef /*searchPath*/,
                            llvm::StringRef /*relativePath*/,
                            const clang::Module * /*imported*/) override
    {
        if (!m_skipInclude && file)
            addSourceFile(file);

        m_skipInclude = false;
    }

    bool FileNotFound(clang::StringRef /*fileNameRef*/, clang::SmallVectorImpl<char> &/*recoveryPath*/) override
    {
        m_skipInclude = true;

        return true;
    }

    void addSourceFile(const clang::FileEntry *file)
    {
        auto filePathId = m_filePathCache.filePathId(
                    FilePath::fromNativeFilePath(absolutePath(file->getName())));

        auto found = std::find(m_sourceFiles.begin(), m_sourceFiles.end(), filePathId);

        if (found == m_sourceFiles.end() || *found != filePathId)
            m_sourceFiles.insert(found, filePathId);
    }

private:
    FilePathIds &m_sourceFiles;
    FilePathCachingInterface &m_filePathCache;
    bool m_skipInclude = false;
};

} // namespace ClangBackEnd
