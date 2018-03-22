/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "indexdataconsumer.h"

#include <clang/Basic/SourceLocation.h>
#include <clang/Index/IndexSymbol.h>
#include <clang/AST/Decl.h>

#include <llvm/ADT/ArrayRef.h>

namespace ClangBackEnd {

namespace  {
bool hasSymbolRole(clang::index::SymbolRole role, clang::index::SymbolRoleSet roleSet)
{
    return roleSet & static_cast<clang::index::SymbolRoleSet>(role);
}

Utils::SmallString symbolName(const clang::NamedDecl *declaration)
{
    return declaration->getName();
}

SymbolType symbolType(clang::index::SymbolRoleSet roles)
{
    if (hasSymbolRole(clang::index::SymbolRole::Reference, roles))
        return SymbolType::DeclarationReference;
    else if (hasSymbolRole(clang::index::SymbolRole::Declaration, roles))
        return SymbolType::Declaration;
    else if (hasSymbolRole(clang::index::SymbolRole::Definition, roles))
        return SymbolType::Definition;

    return SymbolType::None;
}

}

bool IndexDataConsumer::handleDeclOccurence(const clang::Decl *declaration,
                                            clang::index::SymbolRoleSet symbolRoles,
                                            llvm::ArrayRef<clang::index::SymbolRelation> symbolRelations,
                                            clang::FileID fileId,
                                            unsigned offset,
                                            IndexDataConsumer::ASTNodeInfo astNodeInfo)
{

    const auto *namedDeclaration = clang::dyn_cast<clang::NamedDecl>(declaration);
    if (namedDeclaration) {
        if (!namedDeclaration->getIdentifier())
            return true;

        SymbolIndex globalId = toSymbolIndex(declaration->getCanonicalDecl());
        clang::SourceLocation sourceLocation = m_sourceManager->getLocForStartOfFile(fileId).getLocWithOffset(offset);

        auto found = m_symbolEntries.find(globalId);
        if (found == m_symbolEntries.end()) {
            Utils::optional<Utils::PathString> usr = generateUSR(namedDeclaration);
            if (usr) {
                m_symbolEntries.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(globalId),
                                        std::forward_as_tuple(std::move(usr.value()), symbolName(namedDeclaration)));
            }
        }

        m_sourceLocationEntries.emplace_back(globalId,
                                             filePathId(sourceLocation),
                                             lineColum(sourceLocation),
                                             symbolType(symbolRoles));
    }

    return true;
}

} // namespace ClangBackEnd
