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

#include "locationsourcefilecallbacks.h"

#include "macropreprocessorcallbacks.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>

#include <memory>

namespace ClangBackEnd {

LocationSourceFileCallbacks::LocationSourceFileCallbacks(uint line,
                                                         uint column,
                                                         FilePathCachingInterface &filePathCache)
    : m_filePathCache(filePathCache),
      m_line(line),
      m_column(column)
{
}

bool LocationSourceFileCallbacks::handleBeginSource(clang::CompilerInstance &compilerInstance)
{
    auto &preprocessor = compilerInstance.getPreprocessor();

    m_macroPreprocessorCallbacks = new MacroPreprocessorCallbacks(m_sourceLocationsContainer,
                                                                  m_symbolName,
                                                                  preprocessor,
                                                                  m_filePathCache,
                                                                  m_line,
                                                                  m_column);

    preprocessor.addPPCallbacks(std::unique_ptr<clang::PPCallbacks>(m_macroPreprocessorCallbacks));

    return true;
}

SourceLocationsContainer LocationSourceFileCallbacks::takeSourceLocations()
{
    return std::move(m_sourceLocationsContainer);
}

Utils::SmallString LocationSourceFileCallbacks::takeSymbolName()
{
    return std::move(m_symbolName);
}

bool LocationSourceFileCallbacks::hasSourceLocations() const
{
    return m_sourceLocationsContainer.hasContent();
}


} // namespace ClangBackEnd
