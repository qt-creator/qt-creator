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
#include <clang/AST/DeclVisitor.h>

#include <llvm/ADT/ArrayRef.h>

namespace ClangBackEnd {

namespace  {
bool hasSymbolRole(clang::index::SymbolRole role, clang::index::SymbolRoleSet roleSet)
{
    return roleSet & static_cast<clang::index::SymbolRoleSet>(role);
}

Utils::SmallString symbolName(const clang::NamedDecl *declaration)
{
    clang::DeclarationName declarationName(declaration->getIdentifier());

    return declarationName.getAsString();
}

SourceLocationKind sourceLocationKind(clang::index::SymbolRoleSet roles)
{
    if (hasSymbolRole(clang::index::SymbolRole::Reference, roles))
        return SourceLocationKind::DeclarationReference;
    else if (hasSymbolRole(clang::index::SymbolRole::Declaration, roles))
        return SourceLocationKind::Declaration;
    else if (hasSymbolRole(clang::index::SymbolRole::Definition, roles))
        return SourceLocationKind::Definition;

    return SourceLocationKind::None;
}

using SymbolKindAndTags = std::pair<SymbolKind, SymbolTags>;

class IndexingDeclVisitor : public clang::ConstDeclVisitor<IndexingDeclVisitor, SymbolKindAndTags>
{
public:
    SymbolKindAndTags VisitEnumDecl(const clang::EnumDecl *declaration)
    {
        return {SymbolKind::Enumeration, {}};;
    }
    SymbolKindAndTags VisitRecordDecl(const clang::RecordDecl *declaration)
    {
        SymbolKindAndTags result = {SymbolKind::Record, {}};
        switch (declaration->getTagKind()) {
        case clang::TTK_Struct: result.second.push_back(SymbolTag::Struct); break;
        case clang::TTK_Interface: result.second.push_back(SymbolTag::MsvcInterface); break;
        case clang::TTK_Union: result.second.push_back(SymbolTag::Union); break;
        case clang::TTK_Class: result.second.push_back(SymbolTag::Class); break;
        case clang::TTK_Enum: break; // this case can never happen in a record but it is there to silent the warning
        }

        return result;
    }

    SymbolKindAndTags VisitFunctionDecl(const clang::FunctionDecl*)
    {
        return {SymbolKind::Function, {}};
    }

    SymbolKindAndTags VisitVarDecl(const clang::VarDecl*)
    {
        return {SymbolKind::Variable, {}};
    }
};

SymbolKindAndTags symbolKindAndTags(const clang::Decl *declaration)
{
    static IndexingDeclVisitor visitor;
    return visitor.Visit(declaration);
}

bool isDeclaration(clang::index::SymbolRoleSet symbolRoles)
{
    using namespace clang::index;

    return symbolRoles & (uint(SymbolRole::Declaration) | uint(SymbolRole::Definition));
}

bool isReference(clang::index::SymbolRoleSet symbolRoles)
{
    using namespace clang::index;

    return symbolRoles & (uint(SymbolRole::Reference) | uint(SymbolRole::Call));
}

}

bool IndexDataConsumer::skipSymbol(clang::FileID fileId, clang::index::SymbolRoleSet symbolRoles)
{
    bool alreadyParsed = isAlreadyParsed(fileId);
    bool isParsedDeclaration = alreadyParsed && isDeclaration(symbolRoles);
    bool isParsedReference = alreadyParsed && !dependentFilesAreModified() && isReference(symbolRoles);

    return isParsedDeclaration || isParsedReference;
}

bool IndexDataConsumer::handleDeclOccurence(const clang::Decl *declaration,
                                            clang::index::SymbolRoleSet symbolRoles,
                                            llvm::ArrayRef<clang::index::SymbolRelation> symbolRelations,
#if LLVM_VERSION_MAJOR >= 7
                                            clang::SourceLocation sourceLocation,
#else
                                            clang::FileID fileId,
                                            unsigned offset,
#endif
                                            IndexDataConsumer::ASTNodeInfo astNodeInfo)
{
    const auto *namedDeclaration = clang::dyn_cast<clang::NamedDecl>(declaration);
    if (namedDeclaration) {
        if (!namedDeclaration->getIdentifier())
            return true;

#if LLVM_VERSION_MAJOR >= 7
        if (skipSymbol(m_sourceManager->getFileID(sourceLocation), symbolRoles))
#else
        if (skipSymbol(fileId, symbolRoles))
#endif
            return true;

        SymbolIndex globalId = toSymbolIndex(declaration->getCanonicalDecl());
#if LLVM_VERSION_MAJOR < 7
        clang::SourceLocation sourceLocation = m_sourceManager->getLocForStartOfFile(fileId).getLocWithOffset(offset);
#endif

        auto found = m_symbolEntries.find(globalId);
        if (found == m_symbolEntries.end()) {
            Utils::optional<Utils::PathString> usr = generateUSR(namedDeclaration);
            if (usr) {
                auto  kindAndTags = symbolKindAndTags(declaration);
                m_symbolEntries.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(globalId),
                                        std::forward_as_tuple(std::move(usr.value()),
                                                              symbolName(namedDeclaration),
                                                              kindAndTags.first,
                                                              kindAndTags.second));
            }
        }

        m_sourceLocationEntries.emplace_back(globalId,
                                             filePathId(sourceLocation),
                                             lineColum(sourceLocation),
                                             sourceLocationKind(symbolRoles));
    }

    return true;
}

} // namespace ClangBackEnd
