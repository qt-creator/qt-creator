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

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Index/USRGeneration.h>
#include <llvm/ADT/SmallVector.h>

#include <vector>

namespace ClangBackEnd {

class FindNamedDeclarationASTVisitor : public clang::RecursiveASTVisitor<FindNamedDeclarationASTVisitor>
{
public:
    explicit FindNamedDeclarationASTVisitor(const clang::SourceManager &sourceManager,
                                            const clang::SourceLocation cursorSourceLocation)
        : m_sourceManager(sourceManager),
          m_cursorSourceLocation(cursorSourceLocation)
    {
    }

    bool shouldVisitTemplateInstantiations() const
    {
        return true;
    }

    bool VisitNamedDecl(const clang::NamedDecl *declaration)
    {
        auto name = declaration->getNameAsString();

        setResultIfCursorIsInBetween(declaration,
                                     declaration->getLocation(),
                                     declaration->getNameAsString().length());

        return true;
    }

    bool VisitDeclRefExpr(const clang::DeclRefExpr *expression)
    {
        if (!iterateNestedNameSpecifierLocation(expression->getQualifierLoc()))
            return false;

        const auto *declaration = expression->getFoundDecl();
        return setResultIfCursorIsInBetween(declaration,
                                            expression->getLocation(),
                                            declaration->getNameAsString().length());
    }

    bool VisitMemberExpr(const clang::MemberExpr *expression)
    {
        const auto *declaration = expression->getFoundDecl().getDecl();
        return setResultIfCursorIsInBetween(declaration,
                                            expression->getMemberLoc(),
                                            declaration->getNameAsString().length());
    }

    std::vector<const clang::NamedDecl*> takeNamedDecl()
    {
        return std::move(m_namedDeclarations);
    }

private:
    bool canSetResult(const clang::NamedDecl *declaration,
                      clang::SourceLocation location)
    {
        return declaration
            && !setResultIfCursorIsInBetween(declaration,
                                                 location,
                                                 declaration->getNameAsString().length());
    }

    bool iterateNestedNameSpecifierLocation(clang::NestedNameSpecifierLoc nameLocation) {
        while (nameLocation) {
            const auto *declaration = nameLocation.getNestedNameSpecifier()->getAsNamespace();
            if (canSetResult(declaration, nameLocation.getLocalBeginLoc()))
                return false;

            nameLocation = nameLocation.getPrefix();
        }

        return true;
    }

    bool isValidLocationWithCursorInside(clang::SourceLocation startLocation,
                                         clang::SourceLocation endLocation)
    {
        return startLocation.isValid()
                && startLocation.isFileID()
                && endLocation.isValid()
                && endLocation.isFileID()
                && isCursorLocationBetween(startLocation, endLocation);
    }

    bool setResultIfCursorIsInBetween(const clang::NamedDecl *declaration,
                                      clang::SourceLocation startLocation,
                                      clang::SourceLocation endLocation)
    {
        bool isValid = isValidLocationWithCursorInside(startLocation, endLocation);

        if (isValid)
            m_namedDeclarations.push_back(declaration);

        return !isValid;
    }

    bool setResultIfCursorIsInBetween(const clang::NamedDecl *declaration,
                                      clang::SourceLocation location,
                                      uint offset) {
        return offset == 0
            || setResultIfCursorIsInBetween(declaration, location, location.getLocWithOffset(offset - 1));
    }

    bool isCursorLocationBetween(const clang::SourceLocation startLocation,
                                 const clang::SourceLocation endLocation)
    {
        return m_cursorSourceLocation == startLocation
            || m_cursorSourceLocation == endLocation
            || (m_sourceManager.isBeforeInTranslationUnit(startLocation, m_cursorSourceLocation)
             && m_sourceManager.isBeforeInTranslationUnit(m_cursorSourceLocation, endLocation));
    }

    std::vector<const clang::NamedDecl*> m_namedDeclarations;
    const clang::SourceManager &m_sourceManager;
    const clang::SourceLocation m_cursorSourceLocation;
};

inline
std::vector<const clang::NamedDecl *> namedDeclarationsAt(const clang::ASTContext &Context,
                                                          const clang::SourceLocation cursorSourceLocation)
{
    const auto &sourceManager = Context.getSourceManager();
    const auto currentFile = sourceManager.getFilename(cursorSourceLocation);

    FindNamedDeclarationASTVisitor visitor(sourceManager, cursorSourceLocation);

    auto declarations = Context.getTranslationUnitDecl()->decls();
    for (auto &currentDeclation : declarations) {
        const auto &fileLocation = currentDeclation->getLocStart();
        const auto &fileName = sourceManager.getFilename(fileLocation);
        if (fileName == currentFile) {
            visitor.TraverseDecl(currentDeclation);
            const auto &namedDeclarations = visitor.takeNamedDecl();

            if (!namedDeclarations.empty())
                return namedDeclarations;
        }
    }

    return std::vector<const clang::NamedDecl *>();
}

inline
USRName USROfDeclaration(const clang::Decl *declaration)
{
    USRName buffer;

    if (declaration == nullptr || clang::index::generateUSRForDecl(declaration, buffer))
        return buffer;

    return buffer;
}

} // namespace ClangBackend
