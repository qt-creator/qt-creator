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

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace ClangBackEnd {

class CollectIncludesAction final : public clang::PreprocessOnlyAction
{
public:
    CollectIncludesAction(Utils::SmallStringVector &includes,
                          const std::vector<uint> &excludedIncludeUID,
                          std::vector<uint> &alreadyIncludedFileUIDs)
        : includes(includes),
          excludedIncludeUID(excludedIncludeUID),
          alreadyIncludedFileUIDs(alreadyIncludedFileUIDs)
    {
    }

    bool BeginSourceFileAction(clang::CompilerInstance &compilerInstance,
                               llvm::StringRef filename) override
    {
      if (clang::PreprocessOnlyAction::BeginSourceFileAction(compilerInstance, filename)) {
          auto &preprocessor = compilerInstance.getPreprocessor();
          auto &headerSearch = preprocessor.getHeaderSearchInfo();

          auto macroPreprocessorCallbacks = new CollectIncludesPreprocessorCallbacks(headerSearch,
                                                                                     includes,
                                                                                     excludedIncludeUID,
                                                                                     alreadyIncludedFileUIDs);

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
    Utils::SmallStringVector &includes;
    const std::vector<uint> &excludedIncludeUID;
    std::vector<uint> &alreadyIncludedFileUIDs;
};

} // namespace ClangBackEnd
