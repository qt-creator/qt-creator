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

#include "clangtool.h"

#include <sourcerangescontainer.h>
#include <dynamicastmatcherdiagnosticcontainer.h>

#include <filepathcachingfwd.h>

namespace clang {
namespace ast_matchers {
namespace dynamic {
class Diagnostics;
}

namespace internal {
class DynTypedMatcher;
}
}

class SourceManager;
}

namespace ClangBackEnd {

class ClangQuery : public ClangTool
{
public:
    ClangQuery(FilePathCachingInterface &m_filePathCache, Utils::SmallString &&m_query={});

    void setQuery(Utils::SmallString &&m_query);

    void findLocations();

    SourceRangesContainer takeSourceRanges();
    DynamicASTMatcherDiagnosticContainers takeDiagnosticContainers();

private:
    void parseDiagnostics(const clang::ast_matchers::dynamic::Diagnostics &diagnostics);
    void matchLocation(const llvm::Optional< clang::ast_matchers::internal::DynTypedMatcher> &optionalStartMatcher,
                       std::unique_ptr<clang::ASTUnit> ast);

private:
    SourceRangesContainer m_sourceRangesContainer;
    Utils::SmallString m_query;
    std::vector<DynamicASTMatcherDiagnosticContainer> m_diagnosticContainers_;
    ClangBackEnd::FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
