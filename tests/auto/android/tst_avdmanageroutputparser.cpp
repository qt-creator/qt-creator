/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "avdmanageroutputparser.h"

#include <QtTest>

using namespace Android;
using namespace Android::Internal;

class tst_AvdManagerOutputParser : public QObject
{
    Q_OBJECT

private slots:
    void parse_data();
    void parse();
};

void tst_AvdManagerOutputParser::parse_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<AndroidDeviceInfoList>("output");
    QTest::addColumn<QStringList>("errorPaths");

    QTest::newRow("none") << "Available Android Virtual Devices:\n"
                          << AndroidDeviceInfoList() << QStringList();

    QTest::newRow("one") << "Available Android Virtual Devices:\n"
                            "    Name: Test\n"
                            "  Device: Galaxy Nexus (Google)\n"
                            "    Path: :Test.avd\n"
                            "  Target: Google APIs (Google Inc.)\n"
                            "          Based on: Android API 30 Tag/ABI: google_apis/x86\n"
                            "  Sdcard: 512 MB\n"
                         << AndroidDeviceInfoList({{"",
                                                    "Test",
                                                    {"x86"},
                                                    -1,
                                                    IDevice::DeviceConnected,
                                                    IDevice::Emulator,
                                                    Utils::FilePath::fromString(":Test.avd")}})
                         << QStringList();

    QTest::newRow("two") << "Available Android Virtual Devices:\n"
                            "    Name: Test\n"
                            "  Device: Galaxy Nexus (Google)\n"
                            "    Path: :Test.avd\n"
                            "  Target: Google APIs (Google Inc.)\n"
                            "          Based on: Android API 30 Tag/ABI: google_apis/x86\n"
                            "  Sdcard: 512 MB\n"
                            "---------\n"
                            "    Name: TestTablet\n"
                            "  Device: 7in WSVGA (Tablet) (Generic)\n"
                            "    Path: :TestTablet.avd\n"
                            "  Target: Google APIs (Google Inc.)\n"
                            "          Based on: Android API 30 Tag/ABI: google_apis/x86\n"
                            "  Sdcard: 256 MB\n"
                         << AndroidDeviceInfoList({{"",
                                                    "Test",
                                                    {"x86"},
                                                    -1,
                                                    IDevice::DeviceConnected,
                                                    IDevice::Emulator,
                                                    Utils::FilePath::fromString(":Test.avd")},
                                                   {"",
                                                    "TestTablet",
                                                    {"x86"},
                                                    -1,
                                                    IDevice::DeviceConnected,
                                                    IDevice::Emulator,
                                                    Utils::FilePath::fromString(":TestTablet.avd")}}
                                                  )
                         << QStringList();
}

void tst_AvdManagerOutputParser::parse()
{
    QFETCH(QString, input);
    QFETCH(AndroidDeviceInfoList, output);
    QFETCH(QStringList, errorPaths);

    QStringList avdErrorPaths;
    const auto result = parseAvdList(input, &avdErrorPaths);
    QCOMPARE(result, output);
    QCOMPARE(avdErrorPaths, errorPaths);
}

QTEST_MAIN(tst_AvdManagerOutputParser)

#include "tst_avdmanageroutputparser.moc"
