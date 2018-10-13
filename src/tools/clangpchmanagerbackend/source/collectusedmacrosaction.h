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

#pragma once

#include <collectusedmacrosandsourcespreprocessorcallbacks.h>

#include <filepathcachingfwd.h>
#include <usedmacro.h>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>

namespace ClangBackEnd {

class CollectUsedMacrosAction final : public clang::PreprocessOnlyAction
{
public:
    CollectUsedMacrosAction(UsedMacros &usedMacros,
                            FilePathCachingInterface &filePathCache,
                            SourcesManager &sourcesManager,
                            SourceDependencies &sourceDependencies,
                            FilePathIds &sourceFiles,
                            FileStatuses &fileStatuses)
        : m_usedMacros(usedMacros),
          m_filePathCache(filePathCache),
          m_sourcesManager(sourcesManager),
          m_sourceDependencies(sourceDependencies),
          m_sourceFiles(sourceFiles),
          m_fileStatuses(fileStatuses)
    {
    }

    bool BeginSourceFileAction(clang::CompilerInstance &compilerInstance) override
    {
      if (clang::PreprocessOnlyAction::BeginSourceFileAction(compilerInstance)) {
          auto &preprocessor = compilerInstance.getPreprocessor();

          preprocessor.SetSuppressIncludeNotFoundError(true);

          auto macroPreprocessorCallbacks = new CollectUsedMacrosAndSourcesPreprocessorCallbacks(
                      m_usedMacros,
                      m_filePathCache,
                      compilerInstance.getSourceManager(),
                      m_sourcesManager,
                      compilerInstance.getPreprocessorPtr(),
                      m_sourceDependencies,
                      m_sourceFiles,
                      m_fileStatuses);

          preprocessor.addPPCallbacks(std::unique_ptr<clang::PPCallbacks>(macroPreprocessorCallbacks));

          return true;
      }

      return false;
    }

    void EndSourceFileAction() override
    {
      clang::PreprocessOnlyAction::EndSourceFileAction();
    }

private:
    UsedMacros &m_usedMacros;
    FilePathCachingInterface &m_filePathCache;
    SourcesManager &m_sourcesManager;
    SourceDependencies &m_sourceDependencies;
    FilePathIds &m_sourceFiles;
    FileStatuses &m_fileStatuses;
};

} // namespace ClangBackEnd
