// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"
#include "../utils/notification.h"

#include <imagecache/taskqueue.h>

namespace {
struct Task
{
    Task(int i)
        : i{i}
    {}

    int i = 5;

    friend bool operator==(Task first, Task second) { return first.i == second.i; }
};

template<typename Matcher>
auto IsTask(Matcher matcher)
{
    return Field(&Task::i, matcher);
}

class TaskQueue : public testing::Test
{
protected:
    Notification notification;
    Notification waitInThread;
    NiceMock<MockFunction<void(Task &task)>> mockDispatchCallback;
    NiceMock<MockFunction<void(Task &task)>> mockCleanCallback;
    using Queue = QmlDesigner::TaskQueue<Task,
                                         decltype(mockDispatchCallback.AsStdFunction()),
                                         decltype(mockCleanCallback.AsStdFunction())>;
};

TEST_F(TaskQueue, add_task_dispatches_task)
{
    Queue queue{mockDispatchCallback.AsStdFunction(), mockCleanCallback.AsStdFunction()};

    EXPECT_CALL(mockDispatchCallback, Call(IsTask(22))).WillRepeatedly([&](Task) {
        notification.notify();
    });

    queue.addTask(22);
    notification.wait();
}

TEST_F(TaskQueue, depatches_task_in_order)
{
    InSequence s;
    Queue queue{mockDispatchCallback.AsStdFunction(), mockCleanCallback.AsStdFunction()};

    EXPECT_CALL(mockDispatchCallback, Call(IsTask(22))).WillRepeatedly([&](Task) {});
    EXPECT_CALL(mockDispatchCallback, Call(IsTask(32))).WillRepeatedly([&](Task) {
        notification.notify();
    });

    queue.addTask(22);
    queue.addTask(32);
    notification.wait();
}

TEST_F(TaskQueue, cleanup_at_destruction)
{
    InSequence s;
    ON_CALL(mockDispatchCallback, Call(IsTask(22))).WillByDefault([&](Task) {
        notification.notify();
        waitInThread.wait();
    });

    EXPECT_CALL(mockCleanCallback, Call(IsTask(32))).WillRepeatedly([&](Task) {});

    {
        Queue queue{mockDispatchCallback.AsStdFunction(), mockCleanCallback.AsStdFunction()};
        queue.addTask(22);
        queue.addTask(32);
        notification.wait();
        waitInThread.notify();
    }
}

TEST_F(TaskQueue, clean_task_in_queue)
{
    InSequence s;
    ON_CALL(mockDispatchCallback, Call(IsTask(22))).WillByDefault([&](Task) {
        notification.notify();
        waitInThread.wait();
    });
    Queue queue{mockDispatchCallback.AsStdFunction(), mockCleanCallback.AsStdFunction()};
    queue.addTask(22);
    queue.addTask(32);

    EXPECT_CALL(mockCleanCallback, Call(IsTask(32))).WillRepeatedly([&](Task) {});

    notification.wait();
    waitInThread.notify();
    queue.clean();
}

} // namespace
