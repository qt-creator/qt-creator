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

#include "findusrforcursoraction.h"

#include "findcursorusr.h"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning( disable : 4100 )
#endif

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include <algorithm>
#include <vector>

namespace ClangBackEnd {

namespace {

std::vector<USRName> collectConstructorUnifiedSymbolResolutions(const clang::CXXRecordDecl *declarations)
{
  std::vector<USRName> unifiedSymbolResolutions;

  const auto constructorDeclarations = declarations->getDefinition()->ctors();

  std::transform(constructorDeclarations.begin(),
                 constructorDeclarations.end(),
                 std::back_inserter(unifiedSymbolResolutions),
                 USROfDeclaration);

  return unifiedSymbolResolutions;
}

void addUnifiedSymbolResolutionsForDeclaration(const std::vector<const clang::NamedDecl *> &declarations,
                                               std::vector<USRName> &usrs)
{

    std::transform(declarations.begin(),
                   declarations.end(),
                   std::back_inserter(usrs),
                   [&] (const clang::NamedDecl *declaration) {
       return  USROfDeclaration(declaration);
    });
}

}

class FindDeclarationsConsumer : public clang::ASTConsumer
{
public:
    FindDeclarationsConsumer(Utils::SmallString &symbolName,
                             std::vector<USRName> &unifiedSymbolResolutions,
                             uint line,
                             uint column)
        : symbolName(symbolName),
          unifiedSymbolResolutions(unifiedSymbolResolutions),
          line(line),
          column(column)
    {
    }

    void HandleTranslationUnit(clang::ASTContext &astContext) override {
        const auto &sourceManager = astContext.getSourceManager();
        const auto cursorSourceLocation = sourceManager.translateLineCol(sourceManager.getMainFileID(),
                                                                         line,
                                                                         column);

        if (cursorSourceLocation.isValid())
            collectUnifiedSymbolResoltions(astContext, cursorSourceLocation);
    }

    void collectUnifiedSymbolResoltions(clang::ASTContext &astContext,
                                        const clang::SourceLocation &cursorSourceLocation)
    {
        const auto foundDeclarations = namedDeclarationsAt(astContext, cursorSourceLocation);

        if (!foundDeclarations.empty()) {
            const auto firstFoundDeclaration = foundDeclarations.front();

            if (const auto *constructorDecl = clang::dyn_cast<clang::CXXConstructorDecl>(firstFoundDeclaration)) {
                const clang::CXXRecordDecl *foundDeclarationParent = constructorDecl->getParent();
                unifiedSymbolResolutions = collectConstructorUnifiedSymbolResolutions(foundDeclarationParent);
            } else if (const auto *destructorDecl = clang::dyn_cast<clang::CXXDestructorDecl>(firstFoundDeclaration)) {
                const clang::CXXRecordDecl *foundDeclarationParent = destructorDecl->getParent();
                unifiedSymbolResolutions = collectConstructorUnifiedSymbolResolutions(foundDeclarationParent);
            } else if (const auto *recordDeclaration = clang::dyn_cast<clang::CXXRecordDecl>(firstFoundDeclaration)) {
                unifiedSymbolResolutions = collectConstructorUnifiedSymbolResolutions(recordDeclaration);
            }

            addUnifiedSymbolResolutionsForDeclaration(foundDeclarations, unifiedSymbolResolutions);
            symbolName = firstFoundDeclaration->getNameAsString();
        }
    }

private:
    Utils::SmallString &symbolName;
    std::vector<USRName> &unifiedSymbolResolutions;
    uint line;
    uint column;
};

std::unique_ptr<clang::ASTConsumer>
USRFindingAction::newASTConsumer() {
  std::unique_ptr<FindDeclarationsConsumer> Consumer(
      new FindDeclarationsConsumer(symbolName, unifiedSymbolResolutions_, line, column));

  return std::move(Consumer);
}

} // namespace ClangBackEnd
