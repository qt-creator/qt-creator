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

#include <utils/smallstring.h>

#include <sourcelocations.h>

#include <algorithm>

namespace ClangRefactoring {

template <typename StatementFactory>
class SymbolQuery
{
    using ReadStatement = typename StatementFactory::ReadStatementType;

public:
    using Location = SourceLocations::Location;
    using Source = SourceLocations::Source;

    SymbolQuery(StatementFactory &statementFactory)
        : m_statementFactory(statementFactory)
    {}

    SourceLocations locationsAt(const Utils::PathString &filePath, uint line, uint utf8Column)
    {
        ReadStatement &locationsStatement = m_statementFactory.selectLocationsForSymbolLocation;

        const std::size_t reserveSize = 128;

        auto locations = locationsStatement.template values<Location, 3>(
                    reserveSize,
                    filePath,
                    line,
                    utf8Column);

        const std::vector<qint64> sourceIds = uniqueSourceIds(locations);

        ReadStatement &sourcesStatement = m_statementFactory.selectSourcePathForId;

        auto sources = sourcesStatement.template values<Source, 2>(
                    reserveSize,
                    sourceIds);

        return {locations, sourcesToHashMap(sources)};
    }

    static
    qint64 sourceId(const Location &location)
    {
        return location.sourceId;
    }

    static
    std::vector<qint64> uniqueSourceIds(const std::vector<Location> &locations)
    {
        std::vector<qint64> ids;
        ids.reserve(locations.size());

        std::transform(locations.begin(),
                       locations.end(),
                       std::back_inserter(ids),
                       sourceId);

        auto newEnd = std::unique(ids.begin(), ids.end());
        ids.erase(newEnd, ids.end());

        return ids;
    }

    static
    std::unordered_map<qint64, Utils::PathString> sourcesToHashMap(const std::vector<Source> &sources)
    {
        std::unordered_map<qint64, Utils::PathString> dictonary;

        for (const Source &source : sources)
            dictonary.emplace(source.sourceId, std::move(source.sourcePath));

        return dictonary;
    }

private:
    StatementFactory &m_statementFactory;
};

} // namespace ClangRefactoring
