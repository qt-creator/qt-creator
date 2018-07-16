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

    auto newKit = std::make_unique<ProjectExplorer::Kit>("dings");
    ProjectExplorer::Kit *newKitPtr = newKit.get();
    ProjectExplorer::KitManager *kitManager = ProjectExplorer::KitManager::instance();
    QVERIFY(kitManager);
    QVERIFY(kitManager->registerKit(std::move(newKit)));

    dialog.setKitId("dings");
    QCOMPARE(dialog.kit(), newKitPtr);

    kitManager->deregisterKit(newKitPtr);
}

} // namespace Internal
} // namespace QmlProfiler
