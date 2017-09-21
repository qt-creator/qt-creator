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

#include "symbollocationfinderaction.h"

#include "sourcelocationsutils.h"
#include "findlocationsofusrs.h"

#include <filepathcachingfwd.h>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>

#include <memory>

namespace ClangBackEnd {

class FindingSymbolsASTConsumer : public clang::ASTConsumer
{
public:
  FindingSymbolsASTConsumer(std::vector<USRName> &unifiedSymbolResolutions,
                            FilePathCachingInterface &filePathCache)
      : m_unifiedSymbolResolutions(unifiedSymbolResolutions),
        m_filePathCache(filePathCache)
  {
  }

  void HandleTranslationUnit(clang::ASTContext &context) override
  {
    std::vector<clang::SourceLocation> sourceLocations;


    auto &&sourceLocationsOfUsr = takeLocationsOfUSRs(m_unifiedSymbolResolutions, context.getTranslationUnitDecl());
    sourceLocations.insert(sourceLocations.end(),
                           sourceLocationsOfUsr.begin(),
                           sourceLocationsOfUsr.end());


    std::sort(sourceLocations.begin(), sourceLocations.end());
    auto newEnd = std::unique(sourceLocations.begin(), sourceLocations.end());
    sourceLocations.erase(newEnd, sourceLocations.end());

    updateSourceLocations(sourceLocations, context.getSourceManager());

  }

  void updateSourceLocations(const std::vector<clang::SourceLocation> &sourceLocations,
                             const clang::SourceManager &sourceManager)
  {
      appendSourceLocationsToSourceLocationsContainer(*m_sourceLocationsContainer,
                                                      sourceLocations,
                                                      sourceManager,
                                                      m_filePathCache);
  }

  void setSourceLocations(ClangBackEnd::SourceLocationsContainer *sourceLocations)
  {
      m_sourceLocationsContainer = sourceLocations;
  }

private:
  ClangBackEnd::SourceLocationsContainer *m_sourceLocationsContainer = nullptr;
  std::vector<USRName> &m_unifiedSymbolResolutions;
  FilePathCachingInterface &m_filePathCache;
};

std::unique_ptr<clang::ASTConsumer> SymbolLocationFinderAction::newASTConsumer()
{
  auto consumer = std::make_unique<FindingSymbolsASTConsumer>(m_unifiedSymbolResolutions_,
                                                              m_filePathCache);

  consumer->setSourceLocations(&m_sourceLocations);

  return std::move(consumer);
}

} // namespace ClangBackEnd
