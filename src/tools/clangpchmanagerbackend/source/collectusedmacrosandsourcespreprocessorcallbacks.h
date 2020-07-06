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
#include <filestatus.h>
#include <sourcedependency.h>
#include <symbolsvisitorbase.h>
#include <usedmacro.h>

#include <utils/smallstringvector.h>

#include <clang/Basic/SourceManager.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>

#include <QFile>
#include <QDir>
#include <QTemporaryDir>

#include <algorithm>

namespace ClangBackEnd {

class CollectUsedMacrosAndSourcesPreprocessorCallbacksBase : public SymbolsVisitorBase
{
public:
    CollectUsedMacrosAndSourcesPreprocessorCallbacksBase(UsedMacros &usedMacros,
                                                         const FilePathCachingInterface &filePathCache,
                                                         const clang::SourceManager &sourceManager,
                                                         std::shared_ptr<clang::Preprocessor> preprocessor,
                                                         SourceDependencies &sourceDependencies,
                                                         FilePathIds &sourceFiles,
                                                         FileStatuses &fileStatuses)
        : SymbolsVisitorBase(filePathCache, &sourceManager, m_filePathIndices)
        , m_usedMacros(usedMacros)
        , m_preprocessor(preprocessor)
        , m_sourceDependencies(sourceDependencies)
        , m_sourceFiles(sourceFiles)
        , m_fileStatuses(fileStatuses)
    {}

    void addSourceFile(const clang::FileEntry *fileEntry)
    {
        auto id = filePathId(fileEntry);

        auto found = std::lower_bound(m_sourceFiles.begin(), m_sourceFiles.end(), id);

        if (found == m_sourceFiles.end() || *found != id)
            m_sourceFiles.insert(found, id);
    }

    void addFileStatus(const clang::FileEntry *fileEntry)
    {
        auto id = filePathId(fileEntry);

        auto found = std::lower_bound(m_fileStatuses.begin(),
                                      m_fileStatuses.end(),
                                      id,
                                      [] (const auto &first, const auto &second) {
            return first.filePathId < second;
        });

        if (found == m_fileStatuses.end() || found->filePathId != id) {
            m_fileStatuses.emplace(found,
                                   id,
                                   fileEntry->getSize(),
                                   fileEntry->getModificationTime());
        }
    }

    void addSourceDependency(const clang::FileEntry *file, clang::SourceLocation includeLocation)
    {
        auto includeFilePathId = filePathId(includeLocation);
        auto includedFilePathId = filePathId(file);

        SourceDependency sourceDependency{includeFilePathId, includedFilePathId};

        auto found = std::lower_bound(m_sourceDependencies.begin(),
                                      m_sourceDependencies.end(),
                                      sourceDependency,
                                      [](auto first, auto second) { return first < second; });

        if (found == m_sourceDependencies.end() || *found != sourceDependency)
            m_sourceDependencies.emplace(found, sourceDependency);
    }

    void mergeUsedMacros()
    {
        m_usedMacros.reserve(m_usedMacros.size() + m_maybeUsedMacros.size());
        auto insertionPoint = m_usedMacros.insert(m_usedMacros.end(),
                                                  m_maybeUsedMacros.begin(),
                                                  m_maybeUsedMacros.end());
        std::inplace_merge(m_usedMacros.begin(), insertionPoint, m_usedMacros.end());
    }

    static void addUsedMacro(UsedMacro &&usedMacro, UsedMacros &usedMacros)
    {
        if (!usedMacro.filePathId.isValid())
            return;

        auto found = std::lower_bound(usedMacros.begin(),
                                      usedMacros.end(), usedMacro);

        if (found == usedMacros.end() || *found != usedMacro)
            usedMacros.insert(found, std::move(usedMacro));
    }

    void addUsedMacro(const clang::Token &macroNameToken,
                      const clang::MacroDefinition &macroDefinition)
    {
        clang::MacroInfo *macroInfo = macroDefinition.getMacroInfo();
        UsedMacro usedMacro{macroNameToken.getIdentifierInfo()->getName(),
                              filePathId(macroNameToken.getLocation())};
        if (macroInfo)
            addUsedMacro(std::move(usedMacro), m_usedMacros);
        else
            addUsedMacro(std::move(usedMacro), m_maybeUsedMacros);
    }

    bool isInSystemHeader(clang::SourceLocation sourceLocation) const
    {
        return m_sourceManager->isInSystemHeader(sourceLocation);
    }

    void filterOutHeaderGuards()
    {
        auto partitionPoint = std::stable_partition(m_maybeUsedMacros.begin(),
                                                    m_maybeUsedMacros.end(),
                                                    [&] (const UsedMacro &usedMacro) {
            llvm::StringRef id{usedMacro.macroName.data(), usedMacro.macroName.size()};
            clang::IdentifierInfo &identifierInfo = m_preprocessor->getIdentifierTable().get(id);
            clang::MacroInfo *macroInfo = m_preprocessor->getMacroInfo(&identifierInfo);
            return !macroInfo || !macroInfo->isUsedForHeaderGuard();
        });

        m_maybeUsedMacros.erase(partitionPoint, m_maybeUsedMacros.end());
    }

private:
    UsedMacros m_maybeUsedMacros;
    FilePathIds m_filePathIndices;
    UsedMacros &m_usedMacros;
    std::shared_ptr<clang::Preprocessor> m_preprocessor;
    SourceDependencies &m_sourceDependencies;
    FilePathIds &m_sourceFiles;
    FileStatuses &m_fileStatuses;
};

class CollectUsedMacrosAndSourcesPreprocessorCallbacks final : public clang::PPCallbacks,
                                                               public CollectUsedMacrosAndSourcesPreprocessorCallbacksBase
{
public:
    using CollectUsedMacrosAndSourcesPreprocessorCallbacksBase::CollectUsedMacrosAndSourcesPreprocessorCallbacksBase;

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
                            const clang::Token & /*includeToken*/,
                            llvm::StringRef /*fileName*/,
                            bool /*isAngled*/,
                            clang::CharSourceRange /*fileNameRange*/,
                            const clang::FileEntry *file,
                            llvm::StringRef /*searchPath*/,
                            llvm::StringRef /*relativePath*/,
                            const clang::Module * /*imported*/,
                            clang::SrcMgr::CharacteristicKind /*fileType*/
                            ) override
    {
        if (!m_skipInclude && file)
            addSourceDependency(file, hashLocation);

        m_skipInclude = false;
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
    }

private:
    bool m_skipInclude = false;
};
} // namespace ClangBackEnd
