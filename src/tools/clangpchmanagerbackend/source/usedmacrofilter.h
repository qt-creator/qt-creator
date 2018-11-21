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

#include "usedmacro.h"
#include "sourceentry.h"

#include <compilermacro.h>

namespace ClangBackEnd {

class UsedMacroFilter
{
public:
    struct Includes
    {
        FilePathIds project;
        FilePathIds system;
    };

    UsedMacroFilter(const SourceEntries &includes, const UsedMacros &usedMacros)
    {
        Includes filteredIncludes = filterIncludes(includes);
        m_systemUsedMacros = filterUsedMarcos(usedMacros, filteredIncludes.system);
        m_projectUsedMacros = filterUsedMarcos(usedMacros, filteredIncludes.project);
    }

    static Includes filterIncludes(const SourceEntries &includes)
    {
        Includes result;
        result.system.reserve(includes.size());
        result.project.reserve(includes.size());

        for (SourceEntry include : includes)
            filterInclude(include, result);

        return result;
    }

    const Utils::PathStringVector &projectUsedMacros() const { return m_projectUsedMacros; }

    const Utils::PathStringVector &systemUsedMacros() const { return m_systemUsedMacros; }

    const CompilerMacros &projectCompilerMacros() const { return m_projectCompilerMacros; }

    const CompilerMacros &systemCompilerMacros() const { return m_systemCompilerMacros; }

    void filter(const CompilerMacros &compilerMacros)
    {
        CompilerMacros indexedCompilerMacro  = compilerMacros;

        std::sort(indexedCompilerMacro.begin(),
                  indexedCompilerMacro.end(),
                  [](const CompilerMacro &first, const CompilerMacro &second) {
                      return std::tie(first.key, first.value) < std::tie(second.key, second.value);
                  });

        m_systemCompilerMacros = filtercompilerMacros(indexedCompilerMacro, m_systemUsedMacros);
        m_projectCompilerMacros = filtercompilerMacros(indexedCompilerMacro, m_projectUsedMacros);
    }

private:
    static void filterInclude(SourceEntry include, Includes &result)
    {
        switch (include.sourceType) {
        case SourceType::TopSystemInclude:
        case SourceType::SystemInclude:
            result.system.emplace_back(include.sourceId);
            break;
        case SourceType::TopProjectInclude:
        case SourceType::ProjectInclude:
            result.project.emplace_back(include.sourceId);
            break;
        case SourceType::UserInclude:
            break;
        }
    }

    static Utils::PathStringVector filterUsedMarcos(const UsedMacros &usedMacros,
                                                    const FilePathIds &filePathId)
    {
        class BackInserterIterator : public std::back_insert_iterator<Utils::PathStringVector>
        {
        public:
            BackInserterIterator(Utils::PathStringVector &container)
                : std::back_insert_iterator<Utils::PathStringVector>(container)
            {}

            BackInserterIterator &operator=(const UsedMacro &usedMacro)
            {
                container->push_back(usedMacro.macroName);

                return *this;
            }

            BackInserterIterator &operator*() { return *this; }
        };

        struct Compare
        {
            bool operator()(const UsedMacro &usedMacro, FilePathId filePathId)
            {
                return usedMacro.filePathId < filePathId;
            }

            bool operator()(FilePathId filePathId, const UsedMacro &usedMacro)
            {
                return filePathId < usedMacro.filePathId;
            }
        };

        Utils::PathStringVector filtertedMacros;
        filtertedMacros.reserve(usedMacros.size());

        std::set_intersection(usedMacros.begin(),
                              usedMacros.end(),
                              filePathId.begin(),
                              filePathId.end(),
                              BackInserterIterator(filtertedMacros),
                              Compare{});

        std::sort(filtertedMacros.begin(), filtertedMacros.end());

        return filtertedMacros;
    }

    static CompilerMacros filtercompilerMacros(const CompilerMacros &indexedCompilerMacro,
                                               const Utils::PathStringVector &usedMacros)
    {
        struct Compare
        {
            bool operator()(const Utils::PathString &usedMacro,
                            const CompilerMacro &compileMacro)
            {
                return usedMacro < compileMacro.key;
            }

            bool operator()(const CompilerMacro &compileMacro,
                            const Utils::PathString &usedMacro)
            {
                return compileMacro.key < usedMacro;
            }
        };

        CompilerMacros filtertedCompilerMacros;
        filtertedCompilerMacros.reserve(indexedCompilerMacro.size());

        std::set_intersection(indexedCompilerMacro.begin(),
                              indexedCompilerMacro.end(),
                              usedMacros.begin(),
                              usedMacros.end(),
                              std::back_inserter(filtertedCompilerMacros),
                              Compare{});

        std::sort(filtertedCompilerMacros.begin(),
                  filtertedCompilerMacros.end(),
                  [](const CompilerMacro &first, const CompilerMacro &second) {
                      return first.index < second.index;
                  });

        return filtertedCompilerMacros;
    }

private:
    Utils::PathStringVector m_projectUsedMacros;
    Utils::PathStringVector m_systemUsedMacros;
    CompilerMacros m_projectCompilerMacros;
    CompilerMacros m_systemCompilerMacros;
};

} // namespace ClangBackEnd
