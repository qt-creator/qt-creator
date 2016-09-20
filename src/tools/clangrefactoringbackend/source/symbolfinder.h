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

#include "findusrforcursoraction.h"
#include "symbollocationfinderaction.h"
#include "sourcefilecallbacks.h"

#include <sourcelocationscontainer.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "clang/Tooling/Refactoring.h"

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <string>
#include <vector>

namespace ClangBackEnd {

struct FileContent
{
    FileContent(const std::string &directory,
                const std::string &fileName,
                const std::string &content,
                const std::vector<std::string> &commandLine)
        : directory(directory),
          fileName(fileName),
          filePath(directory + nativeSeperator + fileName),
          content(content),
          commandLine(commandLine)
    {}

    std::string directory;
    std::string fileName;
    std::string filePath;
    std::string content;
    std::vector<std::string> commandLine;
};

class SymbolFinder
{
public:
    SymbolFinder(uint line, uint column);

    void addFile(std::string &&directory,
                 std::string &&fileName,
                 std::string &&content,
                 std::vector<std::string> &&commandLine);


    void findSymbol();

    Utils::SmallString takeSymbolName();
    const std::vector<USRName> &unifiedSymbolResolutions();
    const SourceLocationsContainer &sourceLocations() const;
    SourceLocationsContainer takeSourceLocations();

private:
    Utils::SmallString symbolName;
    USRFindingAction usrFindingAction;
    SymbolLocationFinderAction symbolLocationFinderAction;
    SourceFileCallbacks sourceFileCallbacks;
    std::vector<FileContent> fileContents;
    ClangBackEnd::SourceLocationsContainer sourceLocations_;
};

} // namespace ClangBackEnd
