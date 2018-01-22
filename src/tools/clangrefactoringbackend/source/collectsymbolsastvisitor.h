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

#include "sourcelocationentry.h"
#include "sourcelocationsutils.h"
#include "symbolentry.h"
#include "symbolsvisitorbase.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <vector>

namespace ClangBackEnd {

class CollectSymbolsASTVisitor : public clang::RecursiveASTVisitor<CollectSymbolsASTVisitor>,
                                 public SymbolsVisitorBase
{
public:
    CollectSymbolsASTVisitor(SymbolEntries &symbolEntries,
                             SourceLocationEntries &sourceLocationEntries,
                             FilePathCachingInterface &filePathCache,
                             const clang::SourceManager &sourceManager)
        : SymbolsVisitorBase(filePathCache, sourceManager),
          m_symbolEntries(symbolEntries),
          m_sourceLocationEntries(sourceLocationEntries)
    {}

    bool shouldVisitTemplateInstantiations() const
    {
        return true;
    }

    bool VisitNamedDecl(const clang::NamedDecl *declaration)
    {
        if (!declaration->getIdentifier())
            return true;

        SymbolIndex globalId = toSymbolIndex(declaration->getCanonicalDecl());
        auto sourceLocation = declaration->getLocation();

        auto found = m_symbolEntries.find(globalId);
        if (found == m_symbolEntries.end()) {
            Utils::optional<Utils::PathString> usr = generateUSR(declaration);
            if (usr) {
                m_symbolEntries.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(globalId),
                                        std::forward_as_tuple(std::move(usr.value()), symbolName(declaration)));
            }
        }

        m_sourceLocationEntries.emplace_back(globalId,
                                             filePathId(sourceLocation),
                                             lineColum(sourceLocation),
                                             SymbolType::Declaration);

        return true;
    }

    bool VisitDeclRefExpr(const clang::DeclRefExpr *expression)
    {
        auto declaration = expression->getFoundDecl();
        SymbolIndex globalId = toSymbolIndex(declaration->getCanonicalDecl());
        auto sourceLocation = expression->getLocation();

        m_sourceLocationEntries.emplace_back(globalId,
                                             filePathId(sourceLocation),
                                             lineColum(sourceLocation),
                                             SymbolType::DeclarationReference);

        return true;
    }

    Utils::SmallString symbolName(const clang::NamedDecl *declaration)
    {
        return declaration->getName();
    }

private:
    SymbolEntries &m_symbolEntries;
    std::unordered_map<uint, FilePathId> m_filePathIndices;
    SourceLocationEntries &m_sourceLocationEntries;
};


} // namespace ClangBackend

