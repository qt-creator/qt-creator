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

#include <collectincludespreprocessorcallbacks.h>

#include <filepathcachingfwd.h>
#include <filepathid.h>

#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>

namespace ClangBackEnd {

class CollectIncludesAction final : public clang::PreprocessOnlyAction
{
public:
    CollectIncludesAction(FilePathIds &includeIds,
                          FilePathIds &topIncludeIds,
                          FilePathCachingInterface &filePathCache,
                          std::vector<uint> &excludedIncludeUID,
                          std::vector<uint> &alreadyIncludedFileUIDs)
        : m_includeIds(includeIds),
          m_topIncludeIds(topIncludeIds),
          m_filePathCache(filePathCache),
          m_excludedIncludeUID(excludedIncludeUID),
          m_alreadyIncludedFileUIDs(alreadyIncludedFileUIDs)
    {
    }

    bool BeginSourceFileAction(clang::CompilerInstance &compilerInstance) override
    {
      if (clang::PreprocessOnlyAction::BeginSourceFileAction(compilerInstance)) {
          auto &preprocessor = compilerInstance.getPreprocessor();
          auto &headerSearch = preprocessor.getHeaderSearchInfo();

          preprocessor.SetSuppressIncludeNotFoundError(true);

          auto macroPreprocessorCallbacks = new CollectIncludesPreprocessorCallbacks(
                      headerSearch,
                      m_includeIds,
                      m_topIncludeIds,
                      m_filePathCache,
                      m_excludedIncludeUID,
                      m_alreadyIncludedFileUIDs,
                      compilerInstance.getSourceManager());

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
    FilePathIds &m_includeIds;
    FilePathIds &m_topIncludeIds;
    FilePathCachingInterface &m_filePathCache;
    std::vector<uint> &m_excludedIncludeUID;
    std::vector<uint> &m_alreadyIncludedFileUIDs;
};

} // namespace ClangBackEnd
