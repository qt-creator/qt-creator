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

#include "clangrefactoringbackend_global.h"
#include "sourcelocationentry.h"
#include "symbolentry.h"
#include "symbolsvisitorbase.h"

#include <filepathcachingfwd.h>

#include <clang/Index/IndexDataConsumer.h>

namespace ClangBackEnd {

class IndexDataConsumer : public clang::index::IndexDataConsumer,
                          public SymbolsVisitorBase
{
public:
    IndexDataConsumer(SymbolEntries &symbolEntries,
                      SourceLocationEntries &sourceLocationEntries,
                      FilePathCachingInterface &filePathCache,
                      SourcesManager &sourcesManager)
        : SymbolsVisitorBase(filePathCache, nullptr, sourcesManager),
          m_symbolEntries(symbolEntries),
          m_sourceLocationEntries(sourceLocationEntries)
    {}

    IndexDataConsumer(const IndexDataConsumer &) = delete;
    IndexDataConsumer &operator=(const IndexDataConsumer &) = delete;

    bool handleDeclOccurence(const clang::Decl *declaration,
                             clang::index::SymbolRoleSet symbolRoles,
                             llvm::ArrayRef<clang::index::SymbolRelation> symbolRelations,
                             clang::SourceLocation sourceLocation,
                             ASTNodeInfo astNodeInfo) override;

private:
    bool skipSymbol(clang::FileID fileId, clang::index::SymbolRoleSet symbolRoles);

private:
    SymbolEntries &m_symbolEntries;
    SourceLocationEntries &m_sourceLocationEntries;
};

} // namespace ClangBackEnd
