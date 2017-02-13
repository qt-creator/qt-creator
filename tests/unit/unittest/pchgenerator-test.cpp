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

#include "googletest.h"

#include "fakeprocess.h"
#include "testenvironment.h"
#include "mockpchgeneratornotifier.h"

#include <pchgenerator.h>

namespace {

using testing::_;
using testing::Contains;
using testing::Eq;
using testing::NiceMock;
using testing::Not;
using testing::PrintToString;
using ClangBackEnd::TaskFinishStatus;

MATCHER_P(ContainsProcess, process,
          std::string(negation ? "isn't" : "is")
          + " process " + PrintToString(process))
{
    auto found = std::find_if(arg.begin(),
                              arg.end(),
                              [&] (const std::unique_ptr<FakeProcess> &processOwner) {
        return processOwner.get() == process;
    });

    return found != arg.end();
}

class PchGenerator : public testing::Test
{
protected:
    TestEnvironment environment;
    NiceMock<MockPchGeneratorNotifier> mockNotifier;
    ClangBackEnd::PchGenerator<FakeProcess> generator{environment, &mockNotifier};
    Utils::SmallStringVector compilerArguments = {"-DXXXX", "-Ifoo"};
    ClangBackEnd::ProjectPartPch projectPartPch{"projectPartId", "/path/to/pch"};
};

TEST_F(PchGenerator, ProcessFinished)
{
    EXPECT_CALL(mockNotifier, taskFinished(TaskFinishStatus::Successfully, std::move(projectPartPch)));

    generator.startTask(compilerArguments.clone(), projectPartPch.clone());
}

TEST_F(PchGenerator, ProcessFinishedForDeferredProcess)
{
    auto process = generator.addTask(compilerArguments.clone(), projectPartPch.clone());
    generator.startTask(compilerArguments.clone(), projectPartPch.clone());

    EXPECT_CALL(mockNotifier, taskFinished(TaskFinishStatus::Successfully, std::move(projectPartPch)))
            .Times(3);

    generator.startTask(compilerArguments.clone(), projectPartPch.clone());
    process->finish();
}

TEST_F(PchGenerator, ProcessSuccessfullyFinished)
{
    EXPECT_CALL(mockNotifier, taskFinished(TaskFinishStatus::Unsuccessfully, std::move(projectPartPch)));

    auto process =  generator.addTask(compilerArguments.clone(), projectPartPch.clone());
    process->finishUnsuccessfully();
}

TEST_F(PchGenerator, ProcessSuccessfullyFinishedByWrongExitCode)
{
    EXPECT_CALL(mockNotifier, taskFinished(TaskFinishStatus::Unsuccessfully, std::move(projectPartPch)));

    auto process =  generator.addTask(compilerArguments.clone(), projectPartPch.clone());
    process->finishUnsuccessfully();
}

TEST_F(PchGenerator, AddTaskAddsProcessToProcesses)
{
    auto process =  generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    ASSERT_THAT(generator.runningProcesses(), ContainsProcess(process));
}

TEST_F(PchGenerator, RemoveProcessAfterFinishingProcess)
{
    auto process =  generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    process->finish();

    ASSERT_THAT(generator.runningProcesses(), Not(ContainsProcess(process)));
}

TEST_F(PchGenerator, ProcessSuccessfullyFinishedByCrash)
{
    EXPECT_CALL(mockNotifier, taskFinished(TaskFinishStatus::Unsuccessfully, std::move(projectPartPch)));

    auto process =  generator.addTask(compilerArguments.clone(), projectPartPch.clone());
    process->finishByCrash();
}

TEST_F(PchGenerator, CreateProcess)
{
    auto process = generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    ASSERT_THAT(generator.runningProcesses(), ContainsProcess(process));
}

TEST_F(PchGenerator, DeleteProcess)
{
    auto process = generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    generator.deleteProcess(process);

    ASSERT_THAT(generator.runningProcesses(), Not(ContainsProcess(process)));
}

TEST_F(PchGenerator, StartProcessApplicationPath)
{
    auto process =  generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    ASSERT_THAT(process->applicationPath(), environment.clangCompilerPath());

}

TEST_F(PchGenerator, SetCompilerArguments)
{
    auto process =  generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    ASSERT_THAT(process->arguments(), compilerArguments);
}

TEST_F(PchGenerator, ProcessIsStartedAfterAddingTask)
{
    auto process = generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    ASSERT_TRUE(process->isStarted());
}

TEST_F(PchGenerator, DeferProcess)
{
    generator.addTask(compilerArguments.clone(), projectPartPch.clone());
    generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    auto deferProcess = generator.deferProcess();

    ASSERT_TRUE(deferProcess);
}

TEST_F(PchGenerator, ThirdTaskIsDeferred)
{
    generator.addTask(compilerArguments.clone(), projectPartPch.clone());
    generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    auto process = generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    ASSERT_THAT(process, generator.deferredProcesses().back().get());
}

TEST_F(PchGenerator, ThirdTaskIsNotRunning)
{
    generator.addTask(compilerArguments.clone(), projectPartPch.clone());
    generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    auto process = generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    ASSERT_THAT(generator.runningProcesses(), Not(ContainsProcess(process)));
}

TEST_F(PchGenerator, DoNotDeferProcess)
{
    generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    auto deferProcess = generator.deferProcess();

    ASSERT_FALSE(deferProcess);
}

TEST_F(PchGenerator, DoNotActivateIfNothingIsDeferred)
{
    generator.activateNextDeferredProcess();
}

TEST_F(PchGenerator, AfterActivationProcessIsRunning)
{
    generator.addTask(compilerArguments.clone(), projectPartPch.clone());
    generator.addTask(compilerArguments.clone(), projectPartPch.clone());
    auto process = generator.addTask(compilerArguments.clone(), projectPartPch.clone());

    generator.activateNextDeferredProcess();

    ASSERT_THAT(generator.runningProcesses(), ContainsProcess(process));
}

}
