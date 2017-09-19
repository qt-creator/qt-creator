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

#pragma once

#include "symbolqueryinterface.h"

#include <filepathid.h>
#include <sourcelocations.h>

#include <cpptools/usages.h>

#include <algorithm>

namespace ClangRefactoring {

template <typename StatementFactory>
class SymbolQuery final : public SymbolQueryInterface
{
    using ReadStatement = typename StatementFactory::ReadStatementType;

public:
    SymbolQuery(StatementFactory &statementFactory)
        : m_statementFactory(statementFactory)
    {}

    SourceLocations locationsAt(ClangBackEnd::FilePathId filePathId, int line, int utf8Column) override
    {
        ReadStatement &locationsStatement = m_statementFactory.selectLocationsForSymbolLocation;

        const std::size_t reserveSize = 128;

        return locationsStatement.template values<SourceLocation, 4>(reserveSize,
                                                                     filePathId.fileNameId,
                                                                     line,
                                                                     utf8Column);
    }

    CppTools::Usages sourceUsagesAt(ClangBackEnd::FilePathId filePathId, int line, int utf8Column)
    {
        ReadStatement &locationsStatement = m_statementFactory.selectSourceUsagesForSymbolLocation;

        const std::size_t reserveSize = 128;

        return locationsStatement.template values<CppTools::Usage, 3>(reserveSize,
                                                                      filePathId.fileNameId,
                                                                      line,
                                                                      utf8Column);
    }

private:
    StatementFactory &m_statementFactory;
};

} // namespace ClangRefactoring
