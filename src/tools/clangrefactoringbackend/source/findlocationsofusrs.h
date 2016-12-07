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

#include "findcursorusr.h"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning( disable : 4100 )
#endif

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Index/USRGeneration.h>
#include <llvm/ADT/SmallVector.h>

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include <vector>

namespace ClangBackEnd {

class FindLocationsOfUSRsASTVisitor : public clang::RecursiveASTVisitor<FindLocationsOfUSRsASTVisitor>
{
public:
    explicit FindLocationsOfUSRsASTVisitor(const std::vector<USRName> &unifiedSymbolResolutions)
        : unifiedSymbolResolutions(unifiedSymbolResolutions)
    {
    }

    bool VisitNamedDecl(const clang::NamedDecl *declaration) {
        auto declarationUSR = USROfDeclaration(declaration);

        if (containsUSR(declarationUSR))
            foundLocations.push_back(declaration->getLocation());

        return true;
    }

    bool VisitDeclRefExpr(const clang::DeclRefExpr *expression) {
        const auto *declaration = expression->getFoundDecl();

        iterateNestedNameSpecifierLocation(expression->getQualifierLoc());
        auto declarationUSR = USROfDeclaration(declaration);

        if (containsUSR(declarationUSR))
            foundLocations.push_back(expression->getLocation());

        return true;
    }

    bool VisitMemberExpr(const clang::MemberExpr *expression) {
        const auto *declaration = expression->getFoundDecl().getDecl();
        auto declarationUSR = USROfDeclaration(declaration);

        if (containsUSR(declarationUSR))
            foundLocations.push_back(expression->getMemberLoc());

        return true;
    }

    bool shouldVisitTemplateInstantiations() const
    {
        return true;
    }

    std::vector<clang::SourceLocation> takeFoundLocations() const {
        return std::move(foundLocations);
    }

private:
    void iterateNestedNameSpecifierLocation(clang::NestedNameSpecifierLoc nameLocation) {
        while (nameLocation) {
            const auto *declaration = nameLocation.getNestedNameSpecifier()->getAsNamespace();
            if (declaration && containsUSR(USROfDeclaration(declaration)))
                foundLocations.push_back(nameLocation.getLocalBeginLoc());

            nameLocation = nameLocation.getPrefix();
        }
    }

    bool containsUSR(const USRName &unifiedSymbolResolution)
    {
        auto found = std::find(unifiedSymbolResolutions.cbegin(),
                               unifiedSymbolResolutions.cend(),
                               unifiedSymbolResolution);

        return found != unifiedSymbolResolutions.cend();
    }

private:

    // All the locations of the USR were found.
    const std::vector<USRName> unifiedSymbolResolutions;
    std::vector<clang::SourceLocation> foundLocations;
};

inline
std::vector<clang::SourceLocation> takeLocationsOfUSRs(std::vector<USRName> &unifiedSymbolResolutions,
                                                     clang::Decl *declartation)
{
  FindLocationsOfUSRsASTVisitor visitor(unifiedSymbolResolutions);

  visitor.TraverseDecl(declartation);

  return visitor.takeFoundLocations();
}

} // namespace ClangBackend
