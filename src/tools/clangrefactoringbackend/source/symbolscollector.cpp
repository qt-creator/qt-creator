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

#include "symbolscollector.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PreprocessorOptions.h>

namespace ClangBackEnd {

SymbolsCollector::SymbolsCollector(FilePathCaching &filePathCache)
    : m_filePathCache(filePathCache)
    , m_indexDataConsumer(std::make_shared<IndexDataConsumer>(m_symbolEntries,
                                                              m_sourceLocationEntries,
                                                              m_filePathCache,
                                                              m_symbolSourcesManager,
                                                              m_macroSourcesManager))
    , m_collectSymbolsAction(m_indexDataConsumer)
{
}

void SymbolsCollector::addFiles(const FilePathIds &filePathIds,
                                const Utils::SmallStringVector &arguments)
{
    m_clangTool.addFiles(m_filePathCache.filePaths(filePathIds), arguments);
}

void SymbolsCollector::setFile(FilePathId filePathId, const Utils::SmallStringVector &arguments)
{
    addFiles({filePathId}, arguments);
}

void SymbolsCollector::setUnsavedFiles(const V2::FileContainers &unsavedFiles)
{
    m_clangTool.addUnsavedFiles(unsavedFiles);
}

void SymbolsCollector::clear()
{
    m_symbolEntries.clear();
    m_sourceLocationEntries.clear();
    m_clangTool = ClangTool();
}

std::unique_ptr<clang::tooling::FrontendActionFactory> newFrontendActionFactory(CollectSymbolsAction *action)
{
    class FrontendActionFactoryAdapter : public clang::tooling::FrontendActionFactory
    {
    public:
        explicit FrontendActionFactoryAdapter(CollectSymbolsAction *consumerFactory)
            : m_action(consumerFactory)
        {}

        std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<AdaptorAction>(m_action); }

    private:
        class AdaptorAction : public clang::ASTFrontendAction
        {
        public:
            AdaptorAction(CollectSymbolsAction *action)
                : m_action(action)
            {}

            std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &instance,
                                                                  llvm::StringRef inFile) override
            {
                return m_action->newASTConsumer(instance, inFile);
            }

            bool BeginInvocation(clang::CompilerInstance &instance) override
            {
                return m_action->BeginInvocation(instance);
            }
            bool BeginSourceFileAction(clang::CompilerInstance &instance) override
            {
                return m_action->BeginSourceFileAction(instance);
            }

            bool PrepareToExecuteAction(clang::CompilerInstance &instance) override
            {
                return m_action->PrepareToExecuteAction(instance);
            }
            void ExecuteAction() override { m_action->ExecuteAction(); }
            void EndSourceFileAction() override { m_action->EndSourceFileAction(); }

            bool hasPCHSupport() const override { return m_action->hasPCHSupport(); }
            bool hasASTFileSupport() const override { return m_action->hasASTFileSupport(); }
            bool hasIRSupport() const override { return false; }
            bool hasCodeCompletionSupport() const override { return false; }

        private:
            CollectSymbolsAction *m_action;
        };
        CollectSymbolsAction *m_action;
    };

    return std::unique_ptr<clang::tooling::FrontendActionFactory>(
        new FrontendActionFactoryAdapter(action));
}

bool SymbolsCollector::collectSymbols()
{
    auto tool = m_clangTool.createTool();

    auto actionFactory = ClangBackEnd::newFrontendActionFactory(&m_collectSymbolsAction);

    bool noErrors = tool.run(actionFactory.get()) != 1;

    m_clangTool = ClangTool();

    return noErrors;
}

void SymbolsCollector::doInMainThreadAfterFinished()
{
}

const SymbolEntries &SymbolsCollector::symbols() const
{
    return m_symbolEntries;
}

const SourceLocationEntries &SymbolsCollector::sourceLocations() const
{
    return m_sourceLocationEntries;
}

bool SymbolsCollector::isUsed() const
{
    return m_isUsed;
}

void SymbolsCollector::setIsUsed(bool isUsed)
{
    m_isUsed = isUsed;
}

} // namespace ClangBackEnd
