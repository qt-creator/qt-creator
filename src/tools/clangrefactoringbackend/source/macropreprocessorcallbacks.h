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

#include "sourcelocationsutils.h"

#include <filepathcachingfwd.h>
#include <sourcelocationscontainer.h>

#include <clang/Basic/SourceManager.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/MacroInfo.h>

#include <QDebug>

namespace ClangBackEnd {

struct MacroDirectiveToken
{
    MacroDirectiveToken(const clang::MacroDirective *macroDirective,
                        const clang::Token &token)
        : macroDirective(macroDirective),
          token(token)
    {}

    const clang::MacroDirective *macroDirective;
    const clang::Token token;
};

class MacroPreprocessorCallbacks : public clang::PPCallbacks
{
public:
    MacroPreprocessorCallbacks(SourceLocationsContainer &m_sourceLocationsContainer,
                               Utils::SmallString &m_symbolName,
                               clang::Preprocessor &m_preprocessor,
                               FilePathCachingInterface &filePathCache,
                               uint m_line,
                               uint m_column);

    void FileChanged(clang::SourceLocation location,
                     FileChangeReason reason,
                     clang::SrcMgr::CharacteristicKind /*fileType*/,
                     clang::FileID /*previousFileIdentifier*/) final
    {
        if (!m_isMainFileEntered) {
            updateLocations();
            updateIsMainFileEntered(location, reason);
        }
    }

    void MacroDefined(const clang::Token &token, const clang::MacroDirective *macroDirective) final
    {
        if (isInMainFile(token)) {
            if (includesCursorPosition(token)) {
                m_sourceLocations.push_back(token.getLocation());
                m_cursorMacroDirective = macroDirective;
                m_symbolName = Utils::SmallString(token.getIdentifierInfo()->getNameStart(),
                                                token.getIdentifierInfo()->getLength());
            }
        }
    }

    void MacroExpands(const clang::Token &token,
                      const clang::MacroDefinition &macroDefinition,
                      clang::SourceRange /*sourceRange*/,
                      const clang::MacroArgs * /*macroArguments*/) final
    {
        if (includesCursorPosition(token)) {
            appendSourceLocations(token, macroDefinition);
            m_cursorMacroDirective = macroDefinition.getLocalDirective();
            m_symbolName = Utils::SmallString(token.getIdentifierInfo()->getNameStart(),
                                            token.getIdentifierInfo()->getLength());
        } else if (isCurrentTokenExpansion(macroDefinition)) {
            m_sourceLocations.push_back(token.getLocation());
        } else if (isBeforeCursorSourceLocation()) {
            m_preCursorMacroDirectiveTokens.emplace_back(macroDefinition.getLocalDirective(), token);
        }
    }

    void EndOfMainFile() final
    {
        appendSourceLocationsToSourceLocationsContainer(m_sourceLocationsContainer,
                                                        m_sourceLocations,
                                                        sourceManager(),
                                                        m_filePathCache);
    }

private:
    void appendSourceLocations(const clang::Token &token,
                               const clang::MacroDefinition &macroDefinition)
    {
        m_sourceLocations.push_back(macroDefinition.getLocalDirective()->getLocation());
        for (const auto &macroDirectiveToken : m_preCursorMacroDirectiveTokens) {
            if (macroDirectiveToken.macroDirective == macroDefinition.getLocalDirective())
                m_sourceLocations.push_back(macroDirectiveToken.token.getLocation());
        }
        m_sourceLocations.push_back(token.getLocation());
    }

    void updateLocations()
    {
        if (m_mainFileSourceLocation.isInvalid()) {
            m_mainFileSourceLocation = sourceManager().getLocForStartOfFile(sourceManager().getMainFileID());
            m_cursorSourceLocation = sourceManager().translateLineCol(sourceManager().getMainFileID(),
                                                                    m_line,
                                                                    m_column);
        }
    }

    void updateIsMainFileEntered(clang::SourceLocation location, FileChangeReason reason)
    {
        if (location == m_mainFileSourceLocation && reason == PPCallbacks::EnterFile)
          m_isMainFileEntered = true;
    }

    const clang::SourceManager &sourceManager() const
    {
        return m_preprocessor.getSourceManager();
    }

    bool isInMainFile(const clang::Token &token)
    {
        return m_isMainFileEntered && sourceManager().isWrittenInMainFile(token.getLocation());
    }

    bool includesCursorPosition(const clang::Token &token)
    {
        auto start = token.getLocation();
        auto end = token.getEndLoc();

        return m_cursorSourceLocation == start
            || m_cursorSourceLocation == end
            || (sourceManager().isBeforeInTranslationUnit(start, m_cursorSourceLocation) &&
                sourceManager().isBeforeInTranslationUnit(m_cursorSourceLocation, end));
    }

    bool isCurrentTokenExpansion(const clang::MacroDefinition &macroDefinition)
    {
        return m_cursorMacroDirective
            && m_cursorMacroDirective == macroDefinition.getLocalDirective();
    }

    bool isBeforeCursorSourceLocation() const
    {
        return !m_cursorMacroDirective;
    }

private:
    std::vector<clang::SourceLocation> m_sourceLocations;
    std::vector<MacroDirectiveToken> m_preCursorMacroDirectiveTokens;
    SourceLocationsContainer &m_sourceLocationsContainer;
    Utils::SmallString &m_symbolName;
    clang::Preprocessor &m_preprocessor;
    const clang::MacroDirective *m_cursorMacroDirective = nullptr;
    clang::SourceLocation m_mainFileSourceLocation;
    clang::SourceLocation m_cursorSourceLocation;
    FilePathCachingInterface &m_filePathCache;
    uint m_line;
    uint m_column;
    bool m_isMainFileEntered = false;
};

} // namespace ClangBackEnd
