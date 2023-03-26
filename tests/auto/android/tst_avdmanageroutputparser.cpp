// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    QTest::addColumn<Utils::FilePaths>("errorPaths");

    QTest::newRow("none") << "Available Android Virtual Devices:\n"
                          << AndroidDeviceInfoList() << Utils::FilePaths();

    QTest::newRow("one") << "Available Android Virtual Devices:\n"
                            "    Name: Test\n"
                            "  Device: Galaxy Nexus (Google)\n"
                            "    Path: :/Test.avd\n"
                            "  Target: Google APIs (Google Inc.)\n"
                            "          Based on: Android API 30 Tag/ABI: google_apis/x86\n"
                            "  Sdcard: 512 MB\n"
                         << AndroidDeviceInfoList({{"",
                                                    "Test",
                                                    {"x86"},
                                                    -1,
                                                    IDevice::DeviceConnected,
                                                    IDevice::Emulator,
                                                    Utils::FilePath::fromString(":/Test.avd")}})
                         << Utils::FilePaths();

    QTest::newRow("two") << "Available Android Virtual Devices:\n"
                            "    Name: Test\n"
                            "  Device: Galaxy Nexus (Google)\n"
                            "    Path: :/Test.avd\n"
                            "  Target: Google APIs (Google Inc.)\n"
                            "          Based on: Android API 30 Tag/ABI: google_apis/x86\n"
                            "  Sdcard: 512 MB\n"
                            "---------\n"
                            "    Name: TestTablet\n"
                            "  Device: 7in WSVGA (Tablet) (Generic)\n"
                            "    Path: :/TestTablet.avd\n"
                            "  Target: Google APIs (Google Inc.)\n"
                            "          Based on: Android API 30 Tag/ABI: google_apis/x86\n"
                            "  Sdcard: 256 MB\n"
                         << AndroidDeviceInfoList({{"",
                                                    "Test",
                                                    {"x86"},
                                                    -1,
                                                    IDevice::DeviceConnected,
                                                    IDevice::Emulator,
                                                    Utils::FilePath::fromString(":/Test.avd")},
                                                   {"",
                                                    "TestTablet",
                                                    {"x86"},
                                                    -1,
                                                    IDevice::DeviceConnected,
                                                    IDevice::Emulator,
                                                    Utils::FilePath::fromString(":/TestTablet.avd")}}
                                                  )
                         << Utils::FilePaths();
}

void tst_AvdManagerOutputParser::parse()
{
    QFETCH(QString, input);
    QFETCH(AndroidDeviceInfoList, output);
    QFETCH(Utils::FilePaths, errorPaths);

    Utils::FilePaths avdErrorPaths;
    const auto result = parseAvdList(input, &avdErrorPaths);
    QCOMPARE(result, output);
    QCOMPARE(avdErrorPaths, errorPaths);
}

QTEST_GUILESS_MAIN(tst_AvdManagerOutputParser)

#include "tst_avdmanageroutputparser.moc"
