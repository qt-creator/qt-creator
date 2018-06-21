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

#include "locationsourcefilecallbacks.h"
#include "symbollocationfinderaction.h"

namespace ClangBackEnd {

SymbolFinder::SymbolFinder(uint line, uint column, FilePathCachingInterface &filePathCache)
    : m_usrFindingAction(line, column),
      m_symbolLocationFinderAction(filePathCache),
      m_sourceFileCallbacks(line, column, filePathCache)
{
}

void SymbolFinder::findSymbol()
{
    clang::tooling::ClangTool tool = createTool();

    tool.run(clang::tooling::newFrontendActionFactory(&m_usrFindingAction, &m_sourceFileCallbacks).get());

    if (m_sourceFileCallbacks.hasSourceLocations()) {
        m_sourceLocations_ = m_sourceFileCallbacks.takeSourceLocations();
        m_symbolName = m_sourceFileCallbacks.takeSymbolName();
    } else {
        m_symbolLocationFinderAction.setUnifiedSymbolResolutions(m_usrFindingAction.takeUnifiedSymbolResolutions());

        tool.run(clang::tooling::newFrontendActionFactory(&m_symbolLocationFinderAction).get());

        m_sourceLocations_ = m_symbolLocationFinderAction.takeSourceLocations();
        m_symbolName = m_usrFindingAction.takeSymbolName();
    }
}

Utils::SmallString SymbolFinder::takeSymbolName()
{
    return std::move(m_symbolName);
}

const std::vector<USRName> &SymbolFinder::unifiedSymbolResolutions()
{
    return m_symbolLocationFinderAction.unifiedSymbolResolutions();
}

const SourceLocationsContainer &SymbolFinder::sourceLocations() const
{
    return m_sourceLocations_;
}

SourceLocationsContainer SymbolFinder::takeSourceLocations()
{
    return std::move(m_sourceLocations_);
}

} // namespace ClangBackEnd
