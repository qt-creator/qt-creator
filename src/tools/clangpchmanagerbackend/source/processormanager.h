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


#include "processormanagerinterface.h"
#include "generatedfiles.h"

#include <memory>

namespace Sqlite {
class Database;
}

namespace ClangBackEnd {

class GeneratedFiles;

template <typename ProcessorType>
class ProcessorManager : public ProcessorManagerInterface
{
public:
    using Processor = ProcessorType;
    ProcessorManager(const GeneratedFiles &generatedFiles)
        : m_generatedFiles(generatedFiles)
    {}

    Processor &unusedProcessor() override
    {
        auto split = std::partition(m_processors.begin(),
                                    m_processors.end(),
                                    [] (const auto &collector) {
            return collector->isUsed();
        });

        auto freeCollectors = std::distance(split, m_processors.end());

        if (freeCollectors > 0)
            return initializedCollector(*split->get());

        m_processors.push_back(createProcessor());

        return  initializedCollector(*m_processors.back().get());
    }

    const std::vector<std::unique_ptr<Processor>> &processors() const
    {
        return m_processors;
    }

protected:
    ~ProcessorManager() = default;
    virtual std::unique_ptr<Processor> createProcessor() const = 0;

private:
    Processor &initializedCollector(Processor &creator)
    {
        creator.setIsUsed(true);
        creator.setUnsavedFiles(m_generatedFiles.fileContainers());
        return creator;
    }


private:
    std::vector<std::unique_ptr<Processor>> m_processors;
    const GeneratedFiles &m_generatedFiles;
};

} // namespace ClangBackEnd
