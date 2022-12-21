// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerattachdialog_test.h"
#include "projectexplorer/kitmanager.h"
#include "projectexplorer/kit.h"
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

QmlProfilerAttachDialogTest::QmlProfilerAttachDialogTest(QObject *parent) : QObject(parent)
{
}

void QmlProfilerAttachDialogTest::testAccessors()
{
    QmlProfilerAttachDialog dialog;

    int port = dialog.port();
    QVERIFY(port >= 0);
    QVERIFY(port < 65536);

    dialog.setPort(4444);
    QCOMPARE(dialog.port(), 4444);

    ProjectExplorer::KitManager *kitManager = ProjectExplorer::KitManager::instance();
    QVERIFY(kitManager);
    ProjectExplorer::Kit * const newKit = ProjectExplorer::KitManager::registerKit({}, "dings");
    QVERIFY(newKit);

    dialog.setKitId("dings");
    QCOMPARE(dialog.kit(), newKit);

    ProjectExplorer::KitManager::deregisterKit(newKit);
}

} // namespace Internal
} // namespace QmlProfiler
