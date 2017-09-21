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

#include "symbolentry.h"
#include "sourcelocationentry.h"

#include <filepathcachinginterface.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Index/USRGeneration.h>
#include <llvm/ADT/SmallVector.h>

#include <vector>

namespace ClangBackEnd {

Utils::SmallStringView toStringView(clang::StringRef stringReference)
{
    return Utils::SmallStringView(stringReference.data(), stringReference.size());
}

class CollectSymbolsASTVisitor : public clang::RecursiveASTVisitor<CollectSymbolsASTVisitor>
{
public:
    CollectSymbolsASTVisitor(SymbolEntries &symbolEntries,
                             SourceLocationEntries &sourceLocationEntries,
                             FilePathCachingInterface &filePathCache,
                             const clang::SourceManager &sourceManager)
        : m_symbolEntries(symbolEntries),
          m_sourceLocationEntries(sourceLocationEntries),
          m_filePathCache(filePathCache),
          m_sourceManager(sourceManager)
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
            m_symbolEntries.emplace(std::piecewise_construct,
                      std::forward_as_tuple(globalId),
                      std::forward_as_tuple(generateUSR(declaration), symbolName(declaration)));
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

    FilePathId filePathId(clang::SourceLocation sourceLocation)
    {
        uint clangFileId = m_sourceManager.getFileID(sourceLocation).getHashValue();

        auto found = m_filePathIndices.find(clangFileId);

        if (found != m_filePathIndices.end())
            return found->second;

        auto filePath = m_sourceManager.getFilename(sourceLocation);

        FilePathId filePathId = m_filePathCache.filePathId(toStringView(filePath));

        m_filePathIndices.emplace(clangFileId, filePathId);

        return filePathId;
    }

    LineColumn lineColum(clang::SourceLocation sourceLocation)
    {
        return {m_sourceManager.getSpellingLineNumber(sourceLocation),
                m_sourceManager.getSpellingColumnNumber(sourceLocation)};
    }

    Utils::PathString generateUSR(const clang::Decl *declaration)
    {
        llvm::SmallVector<char, 128> usr;

        clang::index::generateUSRForDecl(declaration, usr);

        return {usr.data(), usr.size()};
    }

    Utils::SmallString symbolName(const clang::NamedDecl *declaration)
    {
        const llvm::StringRef symbolName{declaration->getName()};

        return {symbolName.data(), symbolName.size()};
    }

    static SymbolIndex toSymbolIndex(const void *pointer)
    {
        return SymbolIndex(reinterpret_cast<std::uintptr_t>(pointer));
    }

private:
    SymbolEntries &m_symbolEntries;
    std::unordered_map<uint, FilePathId> m_filePathIndices;
    SourceLocationEntries &m_sourceLocationEntries;
    FilePathCachingInterface &m_filePathCache;
    const clang::SourceManager &m_sourceManager;
};


} // namespace ClangBackend

