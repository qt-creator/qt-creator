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

#include "symbolqueryinterface.h"

#include <clangsupport/filepathcachingfwd.h>

#include <cpptools/refactoringengineinterface.h>

namespace ClangBackEnd {
class RefactoringClientInterface;
class RefactoringServerInterface;
}

namespace CppTools { class SymbolFinder; }

namespace ClangRefactoring {

class RefactoringEngine : public CppTools::RefactoringEngineInterface
{
public:
    RefactoringEngine(ClangBackEnd::RefactoringServerInterface &server,
                      ClangBackEnd::RefactoringClientInterface &client,
                      ClangBackEnd::FilePathCachingInterface &filePathCache,
                      SymbolQueryInterface &symbolQuery);
    ~RefactoringEngine() override;

    void startLocalRenaming(const CppTools::CursorInEditor &data,
                            CppTools::ProjectPart *projectPart,
                            RenameCallback &&renameSymbolsCallback) override;
    void globalRename(const CppTools::CursorInEditor &data,
                      CppTools::UsagesCallback &&renameUsagesCallback,
                      const QString &) override;
    void findUsages(const CppTools::CursorInEditor &data,
                    CppTools::UsagesCallback &&showUsagesCallback) const override;
    void globalFollowSymbol(const CppTools::CursorInEditor &data,
                            Utils::ProcessLinkCallback &&processLinkCallback,
                            const CPlusPlus::Snapshot &,
                            const CPlusPlus::Document::Ptr &,
                            CppTools::SymbolFinder *,
                            bool) const override;

    bool isRefactoringEngineAvailable() const override;
    void setRefactoringEngineAvailable(bool isAvailable);

    const ClangBackEnd::FilePathCachingInterface &filePathCache() const
    {
        return m_filePathCache;
    }

private:
    CppTools::Usages locationsAt(const CppTools::CursorInEditor &data) const;

    ClangBackEnd::RefactoringServerInterface &m_server;
    ClangBackEnd::RefactoringClientInterface &m_client;
    ClangBackEnd::FilePathCachingInterface &m_filePathCache;

    SymbolQueryInterface &m_symbolQuery;
};

} // namespace ClangRefactoring
