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

#include "filestatus.h"
#include "sourcedependency.h"
#include "sourcelocationsutils.h"
#include "sourcelocationentry.h"
#include "sourcesmanager.h"
#include "symbolentry.h"
#include "usedmacro.h"

#include <collectusedmacrosandsourcespreprocessorcallbacks.h>

#include <filepath.h>
#include <filepathid.h>

#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>

namespace ClangBackEnd {

class CollectMacrosPreprocessorCallbacks final : public clang::PPCallbacks,
                                                 public CollectUsedMacrosAndSourcesPreprocessorCallbacksBase
{
public:
    CollectMacrosPreprocessorCallbacks(SymbolEntries &symbolEntries,
                                       SourceLocationEntries &sourceLocationEntries,
                                       FilePathIds &sourceFiles,
                                       UsedMacros &usedMacros,
                                       FileStatuses &fileStatuses,
                                       SourceDependencies &sourceDependencies,
                                       FilePathCachingInterface &filePathCache,
                                       const clang::SourceManager &sourceManager,
                                       std::shared_ptr<clang::Preprocessor> &&preprocessor,
                                       SourcesManager &sourcesManager)
        : CollectUsedMacrosAndSourcesPreprocessorCallbacksBase(usedMacros,
                                                               filePathCache,
                                                               sourceManager,
                                                               sourcesManager,
                                                               std::move(preprocessor),
                                                               sourceDependencies,
                                                               sourceFiles,
                                                               fileStatuses),
          m_symbolEntries(symbolEntries),
          m_sourceLocationEntries(sourceLocationEntries)
    {
    }

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
                            const clang::Module * /*imported*/
#if LLVM_VERSION_MAJOR >= 7
                            , clang::SrcMgr::CharacteristicKind /*fileType*/
#endif
                            ) override
    {
        if (!m_skipInclude && file)
            addSourceDependency(file, hashLocation);

        m_skipInclude = false;
    }

    bool FileNotFound(clang::StringRef /*fileNameRef*/, clang::SmallVectorImpl<char> &/*recoveryPath*/) override
    {
        m_skipInclude = true;

        return true;
    }

    void Ifndef(clang::SourceLocation,
                const clang::Token &macroNameToken,
                const clang::MacroDefinition &macroDefinition) override
    {
        addUsedMacro(macroNameToken, macroDefinition);
        addMacroAsSymbol(macroNameToken,
                         firstMacroInfo(macroDefinition.getLocalDirective()),
                         SourceLocationKind::MacroUsage);
    }

    void Ifdef(clang::SourceLocation,
               const clang::Token &macroNameToken,
               const clang::MacroDefinition &macroDefinition) override
    {
        addUsedMacro( macroNameToken, macroDefinition);
        addMacroAsSymbol(macroNameToken,
                         firstMacroInfo(macroDefinition.getLocalDirective()),
                         SourceLocationKind::MacroUsage);
    }

    void Defined(const clang::Token &macroNameToken,
                 const clang::MacroDefinition &macroDefinition,
                 clang::SourceRange) override
    {
        addUsedMacro(macroNameToken, macroDefinition);
        addMacroAsSymbol(macroNameToken,
                         firstMacroInfo(macroDefinition.getLocalDirective()),
                         SourceLocationKind::MacroUsage);
    }

    void MacroDefined(const clang::Token &macroNameToken,
                      const clang::MacroDirective *macroDirective) override
    {
        addMacroAsSymbol(macroNameToken, firstMacroInfo(macroDirective), SourceLocationKind::MacroDefinition);
    }

    void MacroUndefined(const clang::Token &macroNameToken,
                        const clang::MacroDefinition &macroDefinition,
                        const clang::MacroDirective *) override
    {
        addMacroAsSymbol(macroNameToken,
                         firstMacroInfo(macroDefinition.getLocalDirective()),
                         SourceLocationKind::MacroUndefinition);
    }

    void MacroExpands(const clang::Token &macroNameToken,
                      const clang::MacroDefinition &macroDefinition,
                      clang::SourceRange,
                      const clang::MacroArgs *) override
    {
        addUsedMacro(macroNameToken, macroDefinition);
        addMacroAsSymbol(macroNameToken,
                         firstMacroInfo(macroDefinition.getLocalDirective()),
                         SourceLocationKind::MacroUsage);
    }

    void EndOfMainFile() override
    {
        filterOutHeaderGuards();
        mergeUsedMacros();
        m_sourcesManager.updateModifiedTimeStamps();
    }

    static const clang::MacroInfo *firstMacroInfo(const clang::MacroDirective *macroDirective)
    {
        if (macroDirective) {
            const clang::MacroDirective *previousDirective = macroDirective;
            do {
                macroDirective = previousDirective;
                previousDirective = macroDirective->getPrevious();
            } while (previousDirective);

            return macroDirective->getMacroInfo();
        }

        return nullptr;
    }

    void addMacroAsSymbol(const clang::Token &macroNameToken,
                          const clang::MacroInfo *macroInfo,
                          SourceLocationKind symbolType)
    {
        clang::SourceLocation sourceLocation = macroNameToken.getLocation();
        if (macroInfo && sourceLocation.isFileID() && !isInSystemHeader(sourceLocation)) {
            FilePathId fileId = filePathId(sourceLocation);
            if (fileId.isValid()) {
                auto macroName = macroNameToken.getIdentifierInfo()->getName();
                SymbolIndex globalId = toSymbolIndex(macroInfo);

                auto found = m_symbolEntries.find(globalId);
                if (found == m_symbolEntries.end()) {
                    Utils::optional<Utils::PathString> usr = generateUSR(macroName, sourceLocation);
                    if (usr) {
                        m_symbolEntries.emplace(std::piecewise_construct,
                                                std::forward_as_tuple(globalId),
                                                std::forward_as_tuple(std::move(*usr),
                                                                      macroName,
                                                                      SymbolKind::Macro));
                    }
                }

                m_sourceLocationEntries.emplace_back(globalId,
                                                     fileId,
                                                     lineColum(sourceLocation),
                                                     symbolType);
            }

        }
    }

private:
    UsedMacros m_maybeUsedMacros;
    std::shared_ptr<clang::Preprocessor> m_preprocessor;
    SymbolEntries &m_symbolEntries;
    SourceLocationEntries &m_sourceLocationEntries;
    bool m_skipInclude = false;
};

} // namespace ClangBackEnd
