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

#include <sourcelocationscontainer.h>

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning( disable : 4100 )
#endif

#include <clang/Basic/SourceManager.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/MacroInfo.h>

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

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
    MacroPreprocessorCallbacks(SourceLocationsContainer &sourceLocationsContainer,
                               Utils::SmallString &symbolName,
                               clang::Preprocessor &preprocessor,
                               uint line,
                               uint column);

    void FileChanged(clang::SourceLocation location,
                     FileChangeReason reason,
                     clang::SrcMgr::CharacteristicKind /*fileType*/,
                     clang::FileID /*previousFileIdentifier*/) final
    {
        if (!isMainFileEntered) {
            updateLocations();
            updateIsMainFileEntered(location, reason);
        }
    }

    void MacroDefined(const clang::Token &token, const clang::MacroDirective *macroDirective) final
    {
        if (isInMainFile(token)) {
            if (includesCursorPosition(token)) {
                sourceLocations.push_back(token.getLocation());
                cursorMacroDirective = macroDirective;
                symbolName = Utils::SmallString(token.getIdentifierInfo()->getNameStart(),
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
            cursorMacroDirective = macroDefinition.getLocalDirective();
            symbolName = Utils::SmallString(token.getIdentifierInfo()->getNameStart(),
                                            token.getIdentifierInfo()->getLength());
        } else if (isCurrentTokenExpansion(macroDefinition)) {
            sourceLocations.push_back(token.getLocation());
        } else if (isBeforeCursorSourceLocation()) {
            preCursorMacroDirectiveTokens.emplace_back(macroDefinition.getLocalDirective(), token);
        }
    }

    void EndOfMainFile() final
    {
        appendSourceLocationsToSourceLocationsContainer(sourceLocationsContainer,
                                                        sourceLocations,
                                                        sourceManager());
    }

private:
    void appendSourceLocations(const clang::Token &token,
                               const clang::MacroDefinition &macroDefinition)
    {
        sourceLocations.push_back(macroDefinition.getLocalDirective()->getLocation());
        for (const auto &macroDirectiveToken : preCursorMacroDirectiveTokens) {
            if (macroDirectiveToken.macroDirective == macroDefinition.getLocalDirective())
                sourceLocations.push_back(macroDirectiveToken.token.getLocation());
        }
        sourceLocations.push_back(token.getLocation());
    }

    void updateLocations()
    {
        if (mainFileSourceLocation.isInvalid()) {
            mainFileSourceLocation = sourceManager().getLocForStartOfFile(sourceManager().getMainFileID());
            cursorSourceLocation = sourceManager().translateLineCol(sourceManager().getMainFileID(),
                                                                    line,
                                                                    column);
        }
    }

    void updateIsMainFileEntered(clang::SourceLocation location, FileChangeReason reason)
    {
        if (location == mainFileSourceLocation && reason == PPCallbacks::EnterFile)
          isMainFileEntered = true;
    }

    const clang::SourceManager &sourceManager() const
    {
        return preprocessor.getSourceManager();
    }

    bool isInMainFile(const clang::Token &token)
    {
        return isMainFileEntered && sourceManager().isWrittenInMainFile(token.getLocation());
    }

    bool includesCursorPosition(const clang::Token &token)
    {
        auto start = token.getLocation();
        auto end = token.getEndLoc();

        return cursorSourceLocation == start
            || cursorSourceLocation == end
            || (sourceManager().isBeforeInTranslationUnit(start, cursorSourceLocation) &&
                sourceManager().isBeforeInTranslationUnit(cursorSourceLocation, end));
    }

    bool isCurrentTokenExpansion(const clang::MacroDefinition &macroDefinition)
    {
        return cursorMacroDirective
            && cursorMacroDirective == macroDefinition.getLocalDirective();
    }

    bool isBeforeCursorSourceLocation() const
    {
        return !cursorMacroDirective;
    }

private:
    std::vector<clang::SourceLocation> sourceLocations;
    std::vector<MacroDirectiveToken> preCursorMacroDirectiveTokens;
    SourceLocationsContainer &sourceLocationsContainer;
    Utils::SmallString &symbolName;
    clang::Preprocessor &preprocessor;
    const clang::MacroDirective *cursorMacroDirective = nullptr;
    clang::SourceLocation mainFileSourceLocation;
    clang::SourceLocation cursorSourceLocation;
    uint line;
    uint column;
    bool isMainFileEntered = false;
};

} // namespace ClangBackEnd
