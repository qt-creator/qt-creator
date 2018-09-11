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

#include <clang/Frontend/FrontendActions.h>

namespace ClangBackEnd {

SymbolsCollector::SymbolsCollector(Sqlite::Database &database)
    : m_filePathCache(database),
      m_indexDataConsumer(std::make_shared<IndexDataConsumer>(m_symbolEntries, m_sourceLocationEntries, m_filePathCache, m_sourcesManager)),
      m_collectSymbolsAction(m_indexDataConsumer),
      m_collectMacrosSourceFileCallbacks(m_symbolEntries, m_sourceLocationEntries, m_filePathCache, m_sourcesManager)
{
}

void SymbolsCollector::addFiles(const FilePathIds &filePathIds,
                                const Utils::SmallStringVector &arguments)
{
    m_clangTool.addFiles(m_filePathCache.filePaths(filePathIds), arguments);
    m_collectMacrosSourceFileCallbacks.addSourceFiles(filePathIds);
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
    m_indexDataConsumer->clear();
    m_collectMacrosSourceFileCallbacks.clear();
    m_symbolEntries.clear();
    m_sourceLocationEntries.clear();
    m_clangTool = ClangTool();
}

template <typename Factory>
std::unique_ptr<clang::tooling::FrontendActionFactory>
newFrontendActionFactory(Factory *consumerFactory,
                         clang::tooling::SourceFileCallbacks *sourceFileCallbacks)
{
    class FrontendActionFactoryAdapter : public clang::tooling::FrontendActionFactory
    {
    public:
        explicit FrontendActionFactoryAdapter(Factory *consumerFactory,
                                              clang::tooling::SourceFileCallbacks *sourceFileCallbacks)
            : m_consumerFactory(consumerFactory),
              m_sourceFileCallbacks(sourceFileCallbacks)
        {}

        clang::FrontendAction *create() override {
            return new ConsumerFactoryAdaptor(m_consumerFactory, m_sourceFileCallbacks);
        }

    private:
        class ConsumerFactoryAdaptor : public clang::ASTFrontendAction {
        public:
            ConsumerFactoryAdaptor(Factory *consumerFactory,
                                   clang::tooling::SourceFileCallbacks *sourceFileCallbacks)
                : m_consumerFactory(consumerFactory),
                  m_sourceFileCallbacks(sourceFileCallbacks)
            {}

            std::unique_ptr<clang::ASTConsumer>
                    CreateASTConsumer(clang::CompilerInstance &instance, StringRef inFile) override {
                return m_consumerFactory->newASTConsumer(instance, inFile);
            }

        protected:
            bool BeginSourceFileAction(clang::CompilerInstance &CI) override {
                if (!clang::ASTFrontendAction::BeginSourceFileAction(CI))
                    return false;
                if (m_sourceFileCallbacks)
                    return m_sourceFileCallbacks->handleBeginSource(CI);
                return true;
            }
            void EndSourceFileAction() override {
                if (m_sourceFileCallbacks)
                    m_sourceFileCallbacks->handleEndSource();
                clang::ASTFrontendAction::EndSourceFileAction();
            }

        private:
            Factory *m_consumerFactory;
            clang::tooling::SourceFileCallbacks *m_sourceFileCallbacks;
        };
        Factory *m_consumerFactory;
        clang::tooling::SourceFileCallbacks *m_sourceFileCallbacks;
    };

  return std::unique_ptr<clang::tooling::FrontendActionFactory>(
      new FrontendActionFactoryAdapter(consumerFactory, sourceFileCallbacks));
}

void SymbolsCollector::collectSymbols()
{
    auto tool = m_clangTool.createTool();

    tool.run(ClangBackEnd::newFrontendActionFactory(&m_collectSymbolsAction,
                                                    &m_collectMacrosSourceFileCallbacks).get());
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

const FilePathIds &SymbolsCollector::sourceFiles() const
{
    return m_collectMacrosSourceFileCallbacks.sourceFiles();
}

const UsedMacros &SymbolsCollector::usedMacros() const
{
    return m_collectMacrosSourceFileCallbacks.usedMacros();
}

const FileStatuses &SymbolsCollector::fileStatuses() const
{
    return m_collectMacrosSourceFileCallbacks.fileStatuses();
}

const SourceDependencies &SymbolsCollector::sourceDependencies() const
{
    return m_collectMacrosSourceFileCallbacks.sourceDependencies();
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
