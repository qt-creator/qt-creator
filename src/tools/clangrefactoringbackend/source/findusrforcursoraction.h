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

#include "clangrefactoringbackend_global.h"

#include <utils/smallstring.h>

#include <clang/Frontend/FrontendAction.h>

namespace clang {
class ASTConsumer;
class CompilerInstance;
class NamedDecl;
}

namespace ClangBackEnd {

class USRFindingAction
{
public:
    USRFindingAction(uint line, uint column)
      : m_line(line),
        m_column(column)
  {
  }

  std::unique_ptr<clang::ASTConsumer> newASTConsumer();

  std::string takeSymbolName()
  {
    return std::string(m_symbolName);
  }

  std::vector<USRName> takeUnifiedSymbolResolutions()
  {
    return std::move(m_unifiedSymbolResolutions);
  }

private:
  Utils::SmallString m_symbolName;
  std::vector<USRName> m_unifiedSymbolResolutions;
  uint m_line;
  uint m_column;
};

} // namespace ClangBackEnd
