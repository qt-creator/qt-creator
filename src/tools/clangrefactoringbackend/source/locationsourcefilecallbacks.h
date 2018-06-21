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

#include <filepathcachingfwd.h>
#include <sourcelocationscontainer.h>

#include <clang/Tooling/Tooling.h>

namespace llvm {
class StringRef;
}

namespace clang {
class CompilerInstance;
}

namespace ClangBackEnd {

class MacroPreprocessorCallbacks;
class SourceLocationsContainer;

class LocationSourceFileCallbacks : public clang::tooling::SourceFileCallbacks
{
public:
    LocationSourceFileCallbacks(uint line, uint column, FilePathCachingInterface &filePathCache);

    bool handleBeginSource(clang::CompilerInstance &compilerInstance) override;

    SourceLocationsContainer takeSourceLocations();
    Utils::SmallString takeSymbolName();

    bool hasSourceLocations() const;

private:
    SourceLocationsContainer m_sourceLocationsContainer;
    Utils::SmallString m_symbolName;
    MacroPreprocessorCallbacks *m_macroPreprocessorCallbacks;
    FilePathCachingInterface &m_filePathCache;
    uint m_line;
    uint m_column;
};

} // namespace ClangBackEnd
