// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "autotest_global.h"
#include "testresult.h"

#include <utils/filepath.h>

#include <QObject>
#include <QString>

#include <functional>

namespace Autotest {

// Tests a plugin finds and runs on its own, shown in the Tests pane beside the
// ones Qt Creator runs itself. The object is the run, and the run ends with it.
// It begins and ends in the main thread, results and output can be reported
// from any thread.
class AUTOTESTSHARED_EXPORT ExternalTestRun
{
public:
    // The cancel handler runs in the main thread when the user stops the run.
    // Without one the Stop button stays disabled while this run lasts.
    explicit ExternalTestRun(const QString &suiteName,
                             const std::function<void()> &cancelHandler = {});
    ~ExternalTestRun();

    ExternalTestRun(const ExternalTestRun &) = delete;
    ExternalTestRun &operator=(const ExternalTestRun &) = delete;

    // False while another test run is going on: nothing is reported then, and
    // this run never started.
    bool isRunning() const { return m_running; }

    // A test's outcome, or ResultType::TestStart for one about to run, so what
    // it says about itself lands underneath it.
    void reportResult(const QString &testName, ResultType type, const QString &message = {},
                      const Utils::FilePath &file = {}, int line = 0, int durationMs = -1);
    void reportOutput(const QString &text);

private:
    const QString m_suiteName;
    QMetaObject::Connection m_cancelConnection;
    bool m_running = false;
};

} // namespace Autotest
