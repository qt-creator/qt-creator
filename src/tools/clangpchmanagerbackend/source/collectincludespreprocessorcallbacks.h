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

#include "collectmacrospreprocessorcallbacks.h"
#include "sourcelocationsutils.h"

#include <filepathcachinginterface.h>
#include <filepathid.h>

#include <utils/smallstringvector.h>

#include <QFile>
#include <QDir>
#include <QTemporaryDir>

#include <algorithm>

namespace ClangBackEnd {

class CollectIncludesPreprocessorCallbacks final : public clang::PPCallbacks,
                                                   public CollectUsedMacrosAndSourcesPreprocessorCallbacksBase
{
public:
    CollectIncludesPreprocessorCallbacks(FilePathIds &includeIds,
                                         FilePathIds &topIncludeIds,
                                         FilePathIds &topsSystemIncludeIds,
                                         const FilePathCachingInterface &filePathCache,
                                         const std::vector<uint> &excludedIncludeUID,
                                         std::vector<uint> &alreadyIncludedFileUIDs,
                                         const clang::SourceManager &sourceManager,
                                         UsedMacros &usedMacros,
                                         SourcesManager &sourcesManager,
                                         std::shared_ptr<clang::Preprocessor> preprocessor,
                                         SourceDependencies &sourceDependencies,
                                         FilePathIds &sourceFiles,
                                         FileStatuses &fileStatuses)
        : CollectUsedMacrosAndSourcesPreprocessorCallbacksBase(usedMacros,
                                                               filePathCache,
                                                               sourceManager,
                                                               sourcesManager,
                                                               preprocessor,
                                                               sourceDependencies,
                                                               sourceFiles,
                                                               fileStatuses),
          m_includeIds(includeIds),
          m_topIncludeIds(topIncludeIds),
          m_topsSystemIncludeIds(topsSystemIncludeIds),
          m_excludedIncludeUID(excludedIncludeUID),
          m_alreadyIncludedFileUIDs(alreadyIncludedFileUIDs)
    {}

    void FileChanged(clang::SourceLocation sourceLocation,
                     clang::PPCallbacks::FileChangeReason reason,
                     clang::SrcMgr::CharacteristicKind,
                     clang::FileID) override
    {
        if (reason == clang::PPCallbacks::EnterFile)
        {
            const clang::FileEntry *fileEntry = m_sourceManager->getFileEntryForID(
                        m_sourceManager->getFileID(sourceLocation));
            if (fileEntry) {
                addFileStatus(fileEntry);
                addSourceFile(fileEntry);
            }
        }
    }

    void InclusionDirective(clang::SourceLocation hashLocation,
                            const clang::Token &/*includeToken*/,
                            llvm::StringRef /*fileName*/,
                            bool /*isAngled*/,
                            clang::CharSourceRange /*fileNameRange*/,
                            const clang::FileEntry *file,
                            llvm::StringRef /*searchPath*/,
                            llvm::StringRef /*relativePath*/,
                            const clang::Module * /*imported*/,
                            clang::SrcMgr::CharacteristicKind fileType) override
    {
        if (!m_skipInclude && file) {
            addSourceDependency(file, hashLocation);
            auto fileUID = file->getUID();
            auto sourceFileUID = m_sourceManager->getFileEntryForID(m_sourceManager->getFileID(hashLocation))->getUID();
            if (isNotInExcludedIncludeUID(fileUID)) {
                auto notAlreadyIncluded = isNotAlreadyIncluded(fileUID);
                if (notAlreadyIncluded.first) {
                    m_alreadyIncludedFileUIDs.insert(notAlreadyIncluded.second, fileUID);
                    FilePath filePath = filePathFromFile(file);
                    if (!filePath.empty()) {
                        FilePathId includeId = m_filePathCache.filePathId(filePath);
                        m_includeIds.emplace_back(includeId);
                        if (isSystem(fileType) && !isInSystemHeader(hashLocation))
                            m_topsSystemIncludeIds.emplace_back(includeId);
                        if (isInExcludedIncludeUID(sourceFileUID))
                            m_topIncludeIds.emplace_back(includeId);
                    }
                }
            }
        }

        m_skipInclude = false;
    }

    bool FileNotFound(clang::StringRef fileNameRef, clang::SmallVectorImpl<char> &recoveryPath) override
    {
        QTemporaryDir temporaryDirectory;
        temporaryDirectory.setAutoRemove(false);
        const QByteArray temporaryDirUtf8 = temporaryDirectory.path().toUtf8();

        const QString fileName = QString::fromUtf8(fileNameRef.data(), int(fileNameRef.size()));
        QString filePath = temporaryDirectory.path() + '/' + fileName;

        ensureDirectory(temporaryDirectory.path(), fileName);
        createFakeFile(filePath);

        recoveryPath.append(temporaryDirUtf8.cbegin(), temporaryDirUtf8.cend());

        m_skipInclude = true;

        return true;
    }


    void Ifndef(clang::SourceLocation,
                const clang::Token &macroNameToken,
                const clang::MacroDefinition &macroDefinition) override
    {
        addUsedMacro(macroNameToken, macroDefinition);
    }

    void Ifdef(clang::SourceLocation,
               const clang::Token &macroNameToken,
               const clang::MacroDefinition &macroDefinition) override
    {
        addUsedMacro( macroNameToken, macroDefinition);
    }

    void Defined(const clang::Token &macroNameToken,
                 const clang::MacroDefinition &macroDefinition,
                 clang::SourceRange) override
    {
        addUsedMacro(macroNameToken, macroDefinition);
    }

    void MacroExpands(const clang::Token &macroNameToken,
                      const clang::MacroDefinition &macroDefinition,
                      clang::SourceRange,
                      const clang::MacroArgs *) override
    {
        addUsedMacro(macroNameToken, macroDefinition);
    }

    void EndOfMainFile() override
    {
        filterOutHeaderGuards();
        mergeUsedMacros();
        m_sourcesManager.updateModifiedTimeStamps();
    }

    void ensureDirectory(const QString &directory, const QString &fileName)
    {
        QStringList directoryEntries = fileName.split('/');
        directoryEntries.pop_back();

        if (!directoryEntries.isEmpty())
            QDir(directory).mkpath(directoryEntries.join('/'));
    }

    void createFakeFile(const QString &filePath)
    {
        QFile fakeFile;
        fakeFile.setFileName(filePath);

        fakeFile.open(QIODevice::ReadWrite);
        fakeFile.close();
    }

    bool isNotInExcludedIncludeUID(uint uid) const
    {
        return !isInExcludedIncludeUID(uid);
    }

    bool isInExcludedIncludeUID(uint uid) const
    {
        return std::binary_search(m_excludedIncludeUID.begin(),
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

    static FilePath filePathFromFile(const clang::FileEntry *file)
    {
        return FilePath::fromNativeFilePath(absolutePath(file->getName()));
    }

private:
    FilePathIds &m_includeIds;
    FilePathIds &m_topIncludeIds;
    FilePathIds &m_topsSystemIncludeIds;
    const std::vector<uint> &m_excludedIncludeUID;
    std::vector<uint> &m_alreadyIncludedFileUIDs;
    bool m_skipInclude = false;
};

} // namespace ClangBackEnd
