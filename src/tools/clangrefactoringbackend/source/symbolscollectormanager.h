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

#include "symbolscollectormanagerinterface.h"
#include "symbolscollectorinterface.h"
#include "generatedfiles.h"

#include <memory>

namespace Sqlite {
class Database;
}

namespace ClangBackEnd {

class GeneratedFiles;
class SymbolsCollector;
template<typename SymbolsCollector>
class SymbolsCollectorManager final : public SymbolsCollectorManagerInterface
{
public:
    SymbolsCollectorManager(Sqlite::Database &database,
                            const GeneratedFiles &generatedFiles)
        : m_database(database),
          m_generatedFiles(generatedFiles)
    {}

    SymbolsCollector &unusedSymbolsCollector() override
    {
        auto split = std::partition(m_collectors.begin(),
                                    m_collectors.end(),
                                    [] (const auto &collector) {
            return collector->isUsed();
        });

        auto freeCollectors = std::distance(split, m_collectors.end());

        if (freeCollectors > 0)
            return initializedCollector(*split->get());

        m_collectors.emplace_back(std::make_unique<SymbolsCollector>(m_database));

        return  initializedCollector(*m_collectors.back().get());
    }

    const std::vector<std::unique_ptr<SymbolsCollector>> &collectors() const
    {
        return m_collectors;
    }

private:
    SymbolsCollector &initializedCollector(SymbolsCollector &collector)
    {
        collector.setIsUsed(true);
        collector.setUnsavedFiles(m_generatedFiles.fileContainers());
        return collector;
    }

private:
    std::vector<std::unique_ptr<SymbolsCollector>> m_collectors;
    Sqlite::Database &m_database;
    const GeneratedFiles &m_generatedFiles;
};

} // namespace ClangBackEnd
