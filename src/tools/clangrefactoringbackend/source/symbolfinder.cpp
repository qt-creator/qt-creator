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

#include "symbolfinder.h"

#include "refactoringcompilationdatabase.h"
#include "sourcefilecallbacks.h"
#include "symbollocationfinderaction.h"

namespace ClangBackEnd {

SymbolFinder::SymbolFinder(uint line, uint column)
    : usrFindingAction(line, column),
      sourceFileCallbacks(line, column)
{
}

namespace {

// use std::filesystem::path if it is supported by all compilers

std::string toNativePath(std::string &&path)
{
#ifdef _WIN32
    std::replace(path.begin(), path.end(), '/', '\\');
#endif

    return std::move(path);
}
}

void SymbolFinder::addFile(std::string &&directory,
                           std::string &&fileName,
                           std::string &&content,
                           std::vector<std::string> &&commandLine)
{
    fileContents.emplace_back(toNativePath(std::move(directory)),
                              std::move(fileName),
                              std::move(content),
                              std::move(commandLine));
}

void SymbolFinder::findSymbol()
{
    RefactoringCompilationDatabase compilationDatabase;

    for (auto &&fileContent : fileContents)
        compilationDatabase.addFile(fileContent.directory, fileContent.fileName, fileContent.commandLine);

    std::vector<std::string> files;

    for (auto &&fileContent : fileContents)
        files.push_back(fileContent.filePath);

    clang::tooling::ClangTool tool(compilationDatabase, files);

    for (auto &&fileContent : fileContents) {
        if (!fileContent.content.empty())
            tool.mapVirtualFile(fileContent.filePath, fileContent.content);
    }

    tool.run(clang::tooling::newFrontendActionFactory(&usrFindingAction, &sourceFileCallbacks).get());

    if (sourceFileCallbacks.hasSourceLocations()) {
        sourceLocations_ = sourceFileCallbacks.takeSourceLocations();
        symbolName = sourceFileCallbacks.takeSymbolName();
    } else {
        symbolLocationFinderAction.setUnifiedSymbolResolutions(usrFindingAction.takeUnifiedSymbolResolutions());

        tool.run(clang::tooling::newFrontendActionFactory(&symbolLocationFinderAction).get());

        sourceLocations_ = symbolLocationFinderAction.takeSourceLocations();
        symbolName = usrFindingAction.takeSymbolName();
    }
}

Utils::SmallString SymbolFinder::takeSymbolName()
{
    return std::move(symbolName);
}

const std::vector<USRName> &SymbolFinder::unifiedSymbolResolutions()
{
    return symbolLocationFinderAction.unifiedSymbolResolutions();
}

const SourceLocationsContainer &SymbolFinder::sourceLocations() const
{
    return sourceLocations_;
}

SourceLocationsContainer SymbolFinder::takeSourceLocations()
{
    return std::move(sourceLocations_);
}

} // namespace ClangBackEnd
