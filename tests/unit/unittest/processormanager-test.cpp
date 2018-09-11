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

#include "googletest.h"

#include "mockprocessor.h"

#include <sqlitedatabase.h>
#include <refactoringdatabaseinitializer.h>
#include <processormanager.h>

namespace {
using ClangBackEnd::ProcessorInterface;

class Manager final : public ClangBackEnd::ProcessorManager<NiceMock<MockProcessor>>
{
public:
    using Processor = NiceMock<MockProcessor>;
    Manager(const ClangBackEnd::GeneratedFiles &generatedFiles)
        : ClangBackEnd::ProcessorManager<NiceMock<MockProcessor>>(generatedFiles)
    {}

protected:
    std::unique_ptr<NiceMock<MockProcessor>> createProcessor() const override
    {
        return std::make_unique<NiceMock<MockProcessor>>();
    }
};

class ProcessorManager : public testing::Test
{
protected:
    ClangBackEnd::GeneratedFiles generatedFiles;
    Manager manager{generatedFiles};
};

TEST_F(ProcessorManager, CreateUnsedProcessor)
{
    manager.unusedProcessor();

    manager.unusedProcessor();

    ASSERT_THAT(manager.processors(), SizeIs(2));
}

TEST_F(ProcessorManager, ReuseUnsedProcessor)
{
    auto &processor = manager.unusedProcessor();
    processor.setIsUsed(false);

    manager.unusedProcessor();

    ASSERT_THAT(manager.processors(), SizeIs(1));
}

TEST_F(ProcessorManager, AsGetNewUnusedProcessorItIsSetUsed)
{
    auto &processor = manager.unusedProcessor();

    ASSERT_TRUE(processor.isUsed());
}

TEST_F(ProcessorManager, AsGetReusedUnusedProcessorItIsSetUsed)
{
    auto &processor = manager.unusedProcessor();
    processor.setIsUsed(false);

    auto &processor2 = manager.unusedProcessor();

    ASSERT_TRUE(processor2.isUsed());
}

TEST_F(ProcessorManager, UnusedProcessorIsInitializedWithUnsavedFiles)
{
    auto &processor = manager.unusedProcessor();

    ASSERT_TRUE(processor.hasUnsavedFiles);
}

TEST_F(ProcessorManager, ReusedProcessorIsInitializedWithUnsavedFiles)
{
    auto &processor = manager.unusedProcessor();
    processor.setIsUsed(false);

    auto &processor2 = manager.unusedProcessor();

    ASSERT_TRUE(processor2.hasUnsavedFiles);
}

}
