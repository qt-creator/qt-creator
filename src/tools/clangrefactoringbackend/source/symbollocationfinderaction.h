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

#include <filepathcachingfwd.h>
#include <sourcelocationscontainer.h>

#include <clang/Tooling/Refactoring.h>

namespace clang {
class ASTConsumer;
}

namespace ClangBackEnd {

class SymbolLocationFinderAction
{
public:
    SymbolLocationFinderAction(FilePathCachingInterface &filePathCache)
        : m_filePathCache(filePathCache)
    {}

    std::unique_ptr<clang::ASTConsumer> newASTConsumer();

    SourceLocationsContainer takeSourceLocations()
    {
        return std::move(m_sourceLocations);
    }

    void setUnifiedSymbolResolutions(std::vector<USRName> &&unifiedSymbolResolutions)
    {
        m_unifiedSymbolResolutions_ = std::move(unifiedSymbolResolutions);
    }

    const std::vector<USRName> &unifiedSymbolResolutions() const
    {
        return m_unifiedSymbolResolutions_;
    }

private:
    ClangBackEnd::SourceLocationsContainer m_sourceLocations;
    std::vector<USRName> m_unifiedSymbolResolutions_;
    FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
