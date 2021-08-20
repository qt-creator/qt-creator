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

#include <cpptools/refactoringengineinterface.h>
#include <cpptools/cppcursorinfo.h>

#include <QFutureWatcher>

namespace ClangBackEnd {
class RefactoringClientInterface;
class RefactoringServerInterface;
}

namespace ClangCodeModel {
namespace Internal {

class RefactoringEngine : public CppTools::RefactoringEngineInterface
{
public:
    void startLocalRenaming(const CppTools::CursorInEditor &data,
                            const CppTools::ProjectPart *projectPart,
                            RenameCallback &&renameSymbolsCallback) override;
    void globalRename(const CppTools::CursorInEditor &cursor, CppTools::UsagesCallback &&callback,
                      const QString &replacement) override;
    void findUsages(const CppTools::CursorInEditor &cursor,
                    CppTools::UsagesCallback &&callback) const override;
    void globalFollowSymbol(const CppTools::CursorInEditor &cursor,
                            ::Utils::ProcessLinkCallback &&callback,
                            const CPlusPlus::Snapshot &snapshot,
                            const CPlusPlus::Document::Ptr &doc,
                            CppTools::SymbolFinder *symbolFinder,
                            bool inNextSplit) const override;

private:
    using FutureCursorWatcher = QFutureWatcher<CppTools::CursorInfo>;
    std::unique_ptr<FutureCursorWatcher> m_watcher;
};

} // namespace Internal
} // namespace ClangRefactoring
