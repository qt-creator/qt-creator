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

SymbolFinder::SymbolFinder(uint line, uint column)
    : usrFindingAction(line, column),
      sourceFileCallbacks(line, column)
{
}

void SymbolFinder::findSymbol()
{
    clang::tooling::ClangTool tool = createTool();

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
