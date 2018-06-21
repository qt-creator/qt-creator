/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <filepathcachinginterface.h>

#include <utils/linecolumn.h>
#include <utils/optional.h>

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Index/USRGeneration.h>
#include <llvm/ADT/SmallVector.h>

namespace ClangBackEnd {

class SymbolsVisitorBase
{
public:
    SymbolsVisitorBase(FilePathCachingInterface &filePathCache,
                       const clang::SourceManager *sourceManager)
        : m_filePathCache(filePathCache),
          m_sourceManager(sourceManager)
    {}

    FilePathId filePathId(clang::SourceLocation sourceLocation)
    {
        clang::FileID clangFileId = m_sourceManager->getFileID(sourceLocation);
        const clang::FileEntry *fileEntry = m_sourceManager->getFileEntryForID(clangFileId);

        return filePathId(fileEntry);
    }

    FilePathId filePathId(const clang::FileEntry *fileEntry)
    {
        if (fileEntry) {
            uint fileId = fileEntry->getUID();

            if (fileId >= m_filePathIndices.size())
                m_filePathIndices.resize(fileId + 1);

            FilePathId &filePathId = m_filePathIndices[fileId];

            if (filePathId.isValid())
                return filePathId;

            auto filePath = fileEntry->getName();
            FilePathId newFilePathId = m_filePathCache.filePathId(FilePath::fromNativeFilePath(absolutePath(filePath)));

            filePathId = newFilePathId;

            return newFilePathId;
        }

        return {};
    }

    Utils::LineColumn lineColum(clang::SourceLocation sourceLocation)
    {
        return {int(m_sourceManager->getSpellingLineNumber(sourceLocation)),
                int(m_sourceManager->getSpellingColumnNumber(sourceLocation))};
    }

    static Utils::optional<Utils::PathString> generateUSR(const clang::Decl *declaration)
    {
        llvm::SmallVector<char, 128> usr;

        Utils::optional<Utils::PathString> usrOptional;

        bool wasNotWorking = clang::index::generateUSRForDecl(declaration, usr);

        if (!wasNotWorking)
            usrOptional.emplace(usr.data(), usr.size());

        return usrOptional;
    }

    Utils::optional<Utils::PathString> generateUSR(clang::StringRef macroName,
                                                   clang::SourceLocation sourceLocation)
    {
        llvm::SmallVector<char, 128> usr;

        Utils::optional<Utils::PathString> usrOptional;

        bool wasNotWorking = clang::index::generateUSRForMacro(macroName,
                                                               sourceLocation,
                                                               *m_sourceManager,
                                                               usr);

        if (!wasNotWorking)
            usrOptional.emplace(usr.data(), usr.size());

        return usrOptional;
    }

    static SymbolIndex toSymbolIndex(const void *pointer)
    {
        return SymbolIndex(reinterpret_cast<std::uintptr_t>(pointer));
    }

    void setSourceManager(const clang::SourceManager *sourceManager)
    {
        m_sourceManager = sourceManager;
    }

protected:
    std::vector<FilePathId> m_filePathIndices;
    FilePathCachingInterface &m_filePathCache;
    const clang::SourceManager *m_sourceManager = nullptr;
};

} // namespace ClangBackend
