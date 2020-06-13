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
#include "indexdataconsumer.h"
#include "sourcelocationentry.h"
#include "symbolentry.h"

#include <utils/smallstring.h>

#include <filepathcachingfwd.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Index/IndexingAction.h>
#include <clang/Lex/PreprocessorOptions.h>

#include <mutex>

namespace ClangBackEnd {

class CollectSymbolsAction : public clang::WrapperFrontendAction
{
public:
    CollectSymbolsAction(std::shared_ptr<IndexDataConsumer> indexDataConsumer)
        : clang::WrapperFrontendAction(
            clang::index::createIndexingAction(indexDataConsumer, createIndexingOptions()))
        , m_indexDataConsumer(indexDataConsumer)
    {}

    std::unique_ptr<clang::ASTConsumer> newASTConsumer(clang::CompilerInstance &compilerInstance,
                                                       llvm::StringRef inFile);

    static clang::index::IndexingOptions createIndexingOptions()
    {
        clang::index::IndexingOptions options;

        options.SystemSymbolFilter = clang::index::IndexingOptions::SystemSymbolFilterKind::None;
        options.IndexFunctionLocals = true;
        options.IndexMacrosInPreprocessor = true;

        return options;
    }

    bool BeginInvocation(clang::CompilerInstance &compilerInstance) override
    {
        m_indexDataConsumer->setSourceManager(&compilerInstance.getSourceManager());
        compilerInstance.getPreprocessorOpts().AllowPCHWithCompilerErrors = true;
        compilerInstance.getDiagnosticOpts().ErrorLimit = 1;

        return clang::WrapperFrontendAction::BeginInvocation(compilerInstance);
    }

    bool BeginSourceFileAction(clang::CompilerInstance &compilerInstance) override
    {
        compilerInstance.getPreprocessor().SetSuppressIncludeNotFoundError(true);

        return clang::WrapperFrontendAction::BeginSourceFileAction(compilerInstance);
    }

    void EndSourceFileAction() override { clang::WrapperFrontendAction::EndSourceFileAction(); }

    bool PrepareToExecuteAction(clang::CompilerInstance &instance) override
    {
        return clang::WrapperFrontendAction::PrepareToExecuteAction(instance);
    }

    void ExecuteAction() override { clang::WrapperFrontendAction::ExecuteAction(); }

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &compilerInstance,
                                                          llvm::StringRef inFile) override
    {
        return WrapperFrontendAction::CreateASTConsumer(compilerInstance, inFile);
    }

private:
    std::shared_ptr<IndexDataConsumer> m_indexDataConsumer;
};

} // namespace ClangBackEnd
