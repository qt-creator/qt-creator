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

#include "builddependency.h"
#include "collectmacrospreprocessorcallbacks.h"
#include "sourcelocationsutils.h"

#include <filepathcachinginterface.h>
#include <filepathid.h>

#include <utils/smallstringvector.h>

#include <llvm/Support/MemoryBuffer.h>

#include <QFile>
#include <QDir>
#include <QTemporaryDir>

#include <algorithm>

namespace ClangBackEnd {

class CollectBuildDependencyPreprocessorCallbacks final
    : public clang::PPCallbacks,
      public CollectUsedMacrosAndSourcesPreprocessorCallbacksBase
{
public:
    CollectBuildDependencyPreprocessorCallbacks(BuildDependency &buildDependency,
                                         const FilePathCachingInterface &filePathCache,
                                         const std::vector<uint> &excludedIncludeUID,
                                         std::vector<uint> &alreadyIncludedFileUIDs,
                                         clang::SourceManager &sourceManager,
                                         SourcesManager &sourcesManager,
                                         std::shared_ptr<clang::Preprocessor> preprocessor)
        : CollectUsedMacrosAndSourcesPreprocessorCallbacksBase(buildDependency.usedMacros,
                                                               filePathCache,
                                                               sourceManager,
                                                               sourcesManager,
                                                               preprocessor,
                                                               buildDependency.sourceDependencies,
                                                               buildDependency.sourceFiles,
                                                               buildDependency.fileStatuses),
          m_buildDependency(buildDependency),
          m_excludedIncludeUID(excludedIncludeUID),
          m_alreadyIncludedFileUIDs(alreadyIncludedFileUIDs)
    {}

    void FileChanged(clang::SourceLocation sourceLocation,
                     clang::PPCallbacks::FileChangeReason reason,
                     clang::SrcMgr::CharacteristicKind,
                     clang::FileID previousFileId) override
    {
        if (reason == clang::PPCallbacks::EnterFile) {
            clang::FileID currentFileId = m_sourceManager->getFileID(sourceLocation);
            if (m_mainFileId.isInvalid()) {
                m_mainFileId = currentFileId;
            } else {
                const clang::FileEntry *fileEntry = m_sourceManager->getFileEntryForID(
                    currentFileId);
                if (fileEntry) {
                    if (previousFileId == m_mainFileId) {
                        uint sourceFileUID = fileEntry->getUID();
                        auto notAlreadyIncluded = isNotAlreadyIncluded(sourceFileUID);
                        if (notAlreadyIncluded.first)
                            m_alreadyIncludedFileUIDs.insert(notAlreadyIncluded.second,
                                                             sourceFileUID);
                    } else {
                        addFileStatus(fileEntry);
                        addSourceFile(fileEntry);
                    }
                }
            }
        }
    }

    void InclusionDirective(clang::SourceLocation hashLocation,
                            const clang::Token & /*includeToken*/,
                            llvm::StringRef /*fileName*/,
                            bool /*isAngled*/,
                            clang::CharSourceRange /*fileNameRange*/,
                            const clang::FileEntry *file,
                            llvm::StringRef /*searchPath*/,
                            llvm::StringRef /*relativePath*/,
                            const clang::Module * /*imported*/,
                            clang::SrcMgr::CharacteristicKind fileType) override
    {
        clang::FileID currentFileId = m_sourceManager->getFileID(hashLocation);
        if (file) {
            if (currentFileId != m_mainFileId) {
                addSourceDependency(file, hashLocation);
                auto fileUID = file->getUID();
                auto sourceFileUID =
                    m_sourceManager
                        ->getFileEntryForID(
                            m_sourceManager->getFileID(hashLocation))
                        ->getUID();
                auto notAlreadyIncluded = isNotAlreadyIncluded(fileUID);
                if (notAlreadyIncluded.first) {
                    m_alreadyIncludedFileUIDs.insert(notAlreadyIncluded.second,
                                                     fileUID);
                    FilePath filePath = filePathFromFile(file);
                    if (!filePath.empty()) {
                        FilePathId includeId =
                            m_filePathCache.filePathId(filePath);

                        time_t lastModified = file->getModificationTime();

                        SourceType sourceType = SourceType::UserInclude;
                        if (isSystem(fileType)) {
                            if (isInSystemHeader(hashLocation))
                                sourceType = SourceType::SystemInclude;
                            else
                                sourceType = SourceType::TopSystemInclude;
                        } else if (isNotInExcludedIncludeUID(fileUID)) {
                            if (isInExcludedIncludeUID(sourceFileUID))
                                sourceType = SourceType::TopProjectInclude;
                            else
                                sourceType = SourceType::ProjectInclude;
                        }

                        addSource({includeId, sourceType, lastModified});
                    }
                }
            } else {
                addSource({m_filePathCache.filePathId(filePathFromFile(file)),
                           SourceType::Source,
                           file->getModificationTime()});
            }
        } else {
            auto sourceFileId = filePathId(hashLocation);
            m_containsMissingIncludes.emplace_back(sourceFileId);
        }
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
        filterOutIncludesWithMissingIncludes();
    }

    static
    void sortAndMakeUnique(FilePathIds &filePathIds)
    {
        std::sort(filePathIds.begin(), filePathIds.end());
        auto newEnd = std::unique(filePathIds.begin(),
                                  filePathIds.end());
        filePathIds.erase(newEnd, filePathIds.end());
    }

    void appendContainsMissingIncludes(const FilePathIds &dependentSourceFilesWithMissingIncludes)
    {
        auto split = m_containsMissingIncludes.insert(
            m_containsMissingIncludes.end(),
            dependentSourceFilesWithMissingIncludes.begin(),
            dependentSourceFilesWithMissingIncludes.end());
        std::inplace_merge(m_containsMissingIncludes.begin(),
                           split,
                           m_containsMissingIncludes.end());
    }

    void removeAlreadyFoundSourcesWithMissingIncludes(FilePathIds &dependentSourceFilesWithMissingIncludes) const
    {
        FilePathIds filteredDependentSourceFilesWithMissingIncludes;
        filteredDependentSourceFilesWithMissingIncludes.reserve(dependentSourceFilesWithMissingIncludes.size());
        std::set_difference(
            dependentSourceFilesWithMissingIncludes.begin(),
            dependentSourceFilesWithMissingIncludes.end(),
            m_containsMissingIncludes.begin(),
            m_containsMissingIncludes.end(),
            std::back_inserter(
                filteredDependentSourceFilesWithMissingIncludes));
        dependentSourceFilesWithMissingIncludes = filteredDependentSourceFilesWithMissingIncludes;
    }

    void collectSourceWithMissingIncludes(FilePathIds containsMissingIncludes,
                                          const SourceDependencies &sourceDependencies)
    {
        if (containsMissingIncludes.empty())
            return;

        class Compare
        {
        public:
            bool operator()(SourceDependency sourceDependency, FilePathId filePathId)
            {
                return sourceDependency.dependencyFilePathId < filePathId;
            }
            bool operator()(FilePathId filePathId, SourceDependency sourceDependency)
            {
                return filePathId < sourceDependency.dependencyFilePathId;
            }
        };

        FilePathIds dependentSourceFilesWithMissingIncludes;
        auto begin = sourceDependencies.begin();
        for (FilePathId sourceWithMissingInclude : containsMissingIncludes) {
            auto range = std::equal_range(begin,
                                          sourceDependencies.end(),
                                          sourceWithMissingInclude,
                                          Compare{});
            std::for_each(range.first, range.second, [&](auto entry) {
                dependentSourceFilesWithMissingIncludes.emplace_back(entry.filePathId);
            });

            begin = range.second;
        }

        sortAndMakeUnique(dependentSourceFilesWithMissingIncludes);
        removeAlreadyFoundSourcesWithMissingIncludes(dependentSourceFilesWithMissingIncludes);
        appendContainsMissingIncludes(dependentSourceFilesWithMissingIncludes);
        collectSourceWithMissingIncludes(dependentSourceFilesWithMissingIncludes,
                                         sourceDependencies);
    }

    void removeSourceWithMissingIncludesFromSources() {
        class Compare
        {
        public:
            bool operator()(SourceEntry entry, FilePathId filePathId)
            {
                return entry.sourceId < filePathId;
            }
            bool operator()(FilePathId filePathId, SourceEntry entry)
            {
                return filePathId < entry.sourceId;
            }
        };

        SourceEntryReferences sourcesWithMissingIncludes;
        sourcesWithMissingIncludes.reserve(m_containsMissingIncludes.size());
        std::set_intersection(m_buildDependency.sources.begin(),
                              m_buildDependency.sources.end(),
                              m_containsMissingIncludes.begin(),
                              m_containsMissingIncludes.end(),
                              std::back_inserter(sourcesWithMissingIncludes),
                              Compare{});
        for (SourceEntryReference entry : sourcesWithMissingIncludes)
            entry.get().hasMissingIncludes = HasMissingIncludes::Yes;
    }

    SourceDependencies sourceDependenciesSortedByDependendFilePathId() const
    {
        auto sourceDependencies = m_buildDependency.sourceDependencies;
        std::sort(sourceDependencies.begin(), sourceDependencies.end(), [](auto first, auto second) {
            return std::tie(first.dependencyFilePathId, first.filePathId)
                   < std::tie(second.dependencyFilePathId, second.filePathId);
        });

        return sourceDependencies;
    }

    void filterOutIncludesWithMissingIncludes()
    {
        sortAndMakeUnique(m_containsMissingIncludes);

        collectSourceWithMissingIncludes(m_containsMissingIncludes,
                                         sourceDependenciesSortedByDependendFilePathId());

        removeSourceWithMissingIncludesFromSources();
    }

    void ensureDirectory(const QString &directory, const QString &fileName)
    {
        QStringList directoryEntries = fileName.split('/');
        directoryEntries.pop_back();

        if (!directoryEntries.isEmpty())
            QDir(directory).mkpath(directoryEntries.join('/'));
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

    void addSource(SourceEntry sourceEntry) {
        auto &sources = m_buildDependency.sources;
        auto found = std::lower_bound(
            sources.begin(),
            sources.end(),
            sourceEntry,
            [](auto first, auto second) { return first < second; });

        if (found == sources.end() || *found != sourceEntry)
            sources.emplace(found, sourceEntry);
    }

private:
    FilePathIds m_containsMissingIncludes;
    BuildDependency &m_buildDependency;
    const std::vector<uint> &m_excludedIncludeUID;
    std::vector<uint> &m_alreadyIncludedFileUIDs;
    clang::FileID m_mainFileId;
};

} // namespace ClangBackEnd
