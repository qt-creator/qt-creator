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
#include "indexdataconsumer.h"
#include "symbolentry.h"

#include <utils/smallstring.h>

#include <filepathcachingfwd.h>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Index/IndexingAction.h>

#include <mutex>

namespace ClangBackEnd {

class CollectSymbolsAction
{
public:
    CollectSymbolsAction(std::shared_ptr<IndexDataConsumer> indexDataConsumer)
        : m_action(indexDataConsumer, createIndexingOptions()),
          m_indexDataConsumer(indexDataConsumer.get())
    {}

    std::unique_ptr<clang::ASTConsumer> newASTConsumer(clang::CompilerInstance &compilerInstance,
                                                       llvm::StringRef inFile);
private:
    class WrappedIndexAction final : public clang::WrapperFrontendAction
    {
    public:
        WrappedIndexAction(std::shared_ptr<clang::index::IndexDataConsumer> indexDataConsumer,
                           clang::index::IndexingOptions indexingOptions)
            : clang::WrapperFrontendAction(
                  clang::index::createIndexingAction(indexDataConsumer, indexingOptions, nullptr))
        {}

        std::unique_ptr<clang::ASTConsumer>
        CreateASTConsumer(clang::CompilerInstance &compilerInstance,
                          llvm::StringRef inFile) override
        {
            return WrapperFrontendAction::CreateASTConsumer(compilerInstance, inFile);
        }
    };

    static
    clang::index::IndexingOptions createIndexingOptions()
    {
        clang::index::IndexingOptions options;

        options.SystemSymbolFilter = clang::index::IndexingOptions::SystemSymbolFilterKind::None;
        options.IndexFunctionLocals = true;

        return options;
    }

private:
    WrappedIndexAction m_action;
    IndexDataConsumer *m_indexDataConsumer;
};

} // namespace ClangBackEnd
