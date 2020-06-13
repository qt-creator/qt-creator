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

#include "collectusedmacrosaction.h"

#include <filepathcachingfwd.h>
#include <usedmacro.h>

#include <clang/Tooling/Tooling.h>

namespace ClangBackEnd {

class CollectUsedMacrosToolActionFactory final : public clang::tooling::FrontendActionFactory
{
public:
    CollectUsedMacrosToolActionFactory(UsedMacros &usedMacros,
                                       FilePathCachingInterface &filePathCache,
                                       SourceDependencies &sourceDependencies,
                                       FilePathIds &sourceFiles,
                                       FileStatuses &fileStatuses)
        : m_usedMacros(usedMacros),
          m_filePathCache(filePathCache),
          m_sourceDependencies(sourceDependencies),
          m_sourceFiles(sourceFiles),
          m_fileStatuses(fileStatuses)
    {}


    bool runInvocation(std::shared_ptr<clang::CompilerInvocation> invocation,
                       clang::FileManager *fileManager,
                       std::shared_ptr<clang::PCHContainerOperations> pchContainerOperations,
                       clang::DiagnosticConsumer *diagnosticConsumer) override
    {
        return clang::tooling::FrontendActionFactory::runInvocation(invocation,
                                                                    fileManager,
                                                                    pchContainerOperations,
                                                                    diagnosticConsumer);
    }

    std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<CollectUsedMacrosAction>(
                    m_usedMacros,
                    m_filePathCache,
                    m_sourceDependencies,
                    m_sourceFiles,
                    m_fileStatuses);
    }

private:
    UsedMacros &m_usedMacros;
    FilePathCachingInterface &m_filePathCache;
    SourceDependencies &m_sourceDependencies;
    FilePathIds &m_sourceFiles;
    FileStatuses &m_fileStatuses;
};

} // namespace ClangBackEnd
