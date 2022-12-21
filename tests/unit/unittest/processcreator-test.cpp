// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include "eventspy.h"

#include <processcreator.h>
#include <processexception.h>
#include <processstartedevent.h>

#include <utils/hostosinfo.h>

#include <QProcess>

#include <future>

using testing::NotNull;

using ClangBackEnd::ProcessCreator;
using ClangBackEnd::ProcessException;
using ClangBackEnd::ProcessStartedEvent;

namespace  {

class ProcessCreator : public testing::Test
{
protected:
    void SetUp();

protected:
    ::ProcessCreator processCreator;
    QStringList m_arguments = {QStringLiteral("connectionName")};
};

TEST_F(ProcessCreator, ProcessIsNotNull)
{
    auto future = processCreator.createProcess();
    auto process = future.get();

    ASSERT_THAT(process.get(), NotNull());
}

TEST_F(ProcessCreator, ProcessIsRunning)
{
    auto future = processCreator.createProcess();
    auto process = future.get();

    ASSERT_THAT(process->state(), QProcess::Running);
}

TEST_F(ProcessCreator, ProcessPathIsNotExisting)
{
    processCreator.setProcessPath(Utils::HostOsInfo::withExecutableSuffix(ECHOSERVER"fail"));

    auto future = processCreator.createProcess();
    ASSERT_THROW(future.get(), ProcessException);
}

TEST_F(ProcessCreator, ProcessStartIsSucessfull)
{
    auto future = processCreator.createProcess();
    ASSERT_NO_THROW(future.get());
}

TEST_F(ProcessCreator, ProcessObserverGetsEvent)
{
    EventSpy eventSpy(ProcessStartedEvent::ProcessStarted);
    processCreator.setObserver(&eventSpy);
    auto future = processCreator.createProcess();

    eventSpy.waitForEvent();
}

TEST_F(ProcessCreator, TemporayPathIsSetForDefaultInitialization)
{
    QString path = processCreator.temporaryDirectory().path();

    ASSERT_THAT(path.size(), Gt(0));
}

TEST_F(ProcessCreator, TemporayPathIsResetted)
{
    std::string oldPath = processCreator.temporaryDirectory().path().toStdString();

    processCreator.resetTemporaryDirectory();

    ASSERT_THAT(processCreator.temporaryDirectory().path().toStdString(),
                AllOf(Not(IsEmpty()), Ne(oldPath)));
}

void ProcessCreator::SetUp()
{
    processCreator.setTemporaryDirectoryPattern("process-XXXXXXX");
    processCreator.resetTemporaryDirectory();
    processCreator.setProcessPath(Utils::HostOsInfo::withExecutableSuffix(ECHOSERVER));
    processCreator.setArguments(m_arguments);
}
}
