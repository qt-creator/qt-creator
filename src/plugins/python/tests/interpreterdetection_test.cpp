// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "interpreterdetection_test.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

void InterpreterDetectionTest::testReportsAnEmptySearch()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // The logger is copied into the detection and outlives this function when the wait
    // below times out, so the lines it writes to must not live on this stack.
    const auto lines = std::make_shared<QStringList>();
    const ToolDetectionLogger logger([lines](const QString &message) { *lines << message; });

    // The signal reaches every tool detection handler, not just this plugin's, so the
    // temporary directory is what keeps the others from doing anything: it is empty, and
    // nothing outside it is searched.
    // A token of 0 leaves the device's task bookkeeping alone, see
    // IDevice::registerToolDetectionTask().
    emit DeviceManager::instance()->toolDetectionRequested(
        ProjectExplorer::Constants::DESKTOP_DEVICE_ID,
        {FilePath::fromUserInput(tmp.path())},
        0,
        logger);

    const auto reported = [lines](const QString &message) {
        return Utils::contains(*lines, [&message](const QString &line) {
            return line.contains(message);
        });
    };

    QTRY_VERIFY(reported("No new Python interpreters found."));
    QVERIFY(reported("Searching for Python interpreters..."));
}

QObject *createInterpreterDetectionTest()
{
    return new InterpreterDetectionTest;
}

} // namespace Python::Internal

#endif // WITH_TESTS
