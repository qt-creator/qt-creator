// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidsdkmanager_test.h"
#include "androidsdkmanager.h"

#include <QTest>

namespace Android::Internal {

AndroidSdkManagerTest::AndroidSdkManagerTest(QObject *parent)
    : QObject(parent)
{}

AndroidSdkManagerTest::~AndroidSdkManagerTest() = default;

void AndroidSdkManagerTest::testAndroidSdkManagerProgressParser_data()
{
    QTest::addColumn<QString>("output");
    QTest::addColumn<int>("progress");
    QTest::addColumn<bool>("foundAssertion");

    // Output of "sdkmanager --licenses", Android SDK Tools version 4.0
    QTest::newRow("Loading local repository")
        << "Loading local repository...                                                     \r"
        << -1
        << false;

    QTest::newRow("Fetch progress (single line)")
        << "[=============                          ] 34% Fetch remote repository...        \r"
        << 34
        << false;

    QTest::newRow("Fetch progress (multi line)")
        << "[=============================          ] 73% Fetch remote repository...        \r"
           "[=============================          ] 75% Fetch remote repository...        \r"
        << 75
        << false;

    QTest::newRow("Some SDK package licenses not accepted")
        << "7 of 7 SDK package licenses not accepted.\n"
        << -1
        << false;

    QTest::newRow("Unaccepted licenses assertion")
        << "\nReview licenses that have not been accepted (y/N)? "
        << -1
        << true;
}

void AndroidSdkManagerTest::testAndroidSdkManagerProgressParser()
{
    QFETCH(QString, output);
    QFETCH(int, progress);
    QFETCH(bool, foundAssertion);

    bool actualFoundAssertion = false;
    const int actualProgress = parseProgress(output, actualFoundAssertion);

    QCOMPARE(progress, actualProgress);
    QCOMPARE(foundAssertion, actualFoundAssertion);
}


}
