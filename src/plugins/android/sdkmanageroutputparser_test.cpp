// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sdkmanageroutputparser_test.h"
#include "sdkmanageroutputparser.h"

#include "androidsdkpackage.h"

#include <QMap>
#include <QTest>
#include <QVersionNumber>

namespace Android::Internal {

class SdkManagerOutputParserTest : public QObject
{
    Q_OBJECT

    AndroidSdkPackageList m_packages;
    SdkManagerOutputParser m_parser{m_packages};

private slots:
    void testParsePackageListing_data();
    void testParsePackageListing();

    void testParseAndroidCliPackageListing();
    void testParseVerboseListingWithoutTrailingNewline();

    void testParseMarkers_data();
    void testParseMarkers();

    void testParseBuildToolsPackage_data();
    void testParseBuildToolsPackage();
    void testParseBuildToolsPackageEmpty();

    void testParseSdkToolsPackage_data();
    void testParseSdkToolsPackage();
    void testParseSdkToolsPackageEmpty();

    void testParsePlatformToolsPackage_data();
    void testParsePlatformToolsPackage();
    void testParsePlatformToolsPackageEmpty();

    void testParseEmulatorToolsPackage_data();
    void testParseEmulatorToolsPackage();
    void testParseEmulatorToolsPackageEmpty();

    void testParseNdkPackage_data();
    void testParseNdkPackage();
    void testParseNdkPackageEmpty();

    void testParseExtraToolsPackage_data();
    void testParseExtraToolsPackage();
    void testParseExtraToolsPackageEmpty();

    void testParseGenericToolsPackage_data();
    void testParseGenericToolsPackage();
    void testParseGenericToolsPackageEmpty();

    void testParsePlatformPackage_data();
    void testParsePlatformPackage();
    void testParsePlatformPackageEmpty();

    void testParseSystemImagePackage_data();
    void testParseSystemImagePackage();
    void testParseSystemImagePackageEmpty();
};

void SdkManagerOutputParserTest::testParseMarkers_data()
{
    QTest::addColumn<QString>("output");
    QTest::addColumn<SdkManagerOutputParser::MarkerTag>("markerTag");

    const QMap<SdkManagerOutputParser::MarkerTag, QString> testData
        = {{SdkManagerOutputParser::MarkerTag::InstalledPackagesMarker, "Installed packages:"},
           {SdkManagerOutputParser::MarkerTag::AvailablePackagesMarkers, "Available Packages:"},
           {SdkManagerOutputParser::MarkerTag::AvailableUpdatesMarker, "Available Updates:"},
           {SdkManagerOutputParser::MarkerTag::EmptyMarker, ""},
           {SdkManagerOutputParser::MarkerTag::PlatformMarker, "platforms"},
           {SdkManagerOutputParser::MarkerTag::SystemImageMarker, "system-images"},
           {SdkManagerOutputParser::MarkerTag::BuildToolsMarker, "build-tools"},
           {SdkManagerOutputParser::MarkerTag::SdkToolsMarker, "tools"},
           {SdkManagerOutputParser::MarkerTag::PlatformToolsMarker, "platform-tools"},
           {SdkManagerOutputParser::MarkerTag::EmulatorToolsMarker, "emulator"},
           {SdkManagerOutputParser::MarkerTag::NdkMarker, "ndk"},
           {SdkManagerOutputParser::MarkerTag::ExtrasMarker, "extras"},
           {SdkManagerOutputParser::MarkerTag::CmdlineSdkToolsMarker, "cmdline-tools"},
           {SdkManagerOutputParser::MarkerTag::GenericToolMarker, "sources;android-32"}};

    for (auto it = testData.begin(); it != testData.end(); ++it)
        QTest::newRow(it->toLatin1().constData()) << *it << it.key();

    QTest::newRow("Installed packages")
        << "Installed packages:" << SdkManagerOutputParser::MarkerTag::InstalledPackagesMarker;
}

void SdkManagerOutputParserTest::testParseMarkers()
{
    QFETCH(QString, output);
    QFETCH(SdkManagerOutputParser::MarkerTag, markerTag);

    SdkManagerOutputParser::MarkerTag actualMarkerTag = m_parser.parseMarkers(output);

    QCOMPARE(actualMarkerTag, markerTag);
}

// BuildTools
void SdkManagerOutputParserTest::testParseBuildToolsPackage_data()
{
    QTest::addColumn<QStringList>("output");

    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("displayText");
    QTest::addColumn<QVersionNumber>("revision");

    QTest::newRow("build-tools;33.0.1")
        << QStringList({"build-tools;33.0.1",
                        "    Description:        Android SDK Build-Tools 33.0.1",
                        "    Version:            33.0.1"})
        << "Android SDK Build-Tools 33.0.1"
        << "Android SDK Build-Tools 33.0.1" << QVersionNumber({33, 0, 1});

    QTest::newRow("build-tools;33.0.3")
        << QStringList({"build-tools;33.0.3",
                        "    Description:        Android SDK Build-Tools 33.0.3",
                        "    Version:            33.0.3"})
        << "Android SDK Build-Tools 33.0.3"
        << "Android SDK Build-Tools 33.0.3" << QVersionNumber({33, 0, 3});
}

void SdkManagerOutputParserTest::testParseBuildToolsPackage()
{
    QFETCH(QStringList, output);
    QFETCH(QString, description);
    QFETCH(QString, displayText);
    QFETCH(QVersionNumber, revision);

    AndroidSdkPackage *actualBuildTools = m_parser.parseBuildToolsPackage(output);

    QVERIFY(actualBuildTools != nullptr);
    QCOMPARE(actualBuildTools->descriptionText(), description);
    QCOMPARE(actualBuildTools->displayText(), displayText);
    QCOMPARE(actualBuildTools->revision(), revision);
}

void SdkManagerOutputParserTest::testParseBuildToolsPackageEmpty()
{
    AndroidSdkPackage *actualBuildTools = m_parser.parseBuildToolsPackage({""});

    QVERIFY(actualBuildTools == nullptr);
}

// SdkTools
void SdkManagerOutputParserTest::testParseSdkToolsPackage_data()
{
    QTest::addColumn<QStringList>("output");

    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("displayText");
    QTest::addColumn<QVersionNumber>("revision");

    QTest::newRow("cmdline-tools;latest")
        << QStringList({"cmdline-tools;latest",
                        "    Description:        Android SDK Command-line Tools (latest)",
                        "    Version:            9.0"})
        << "Android SDK Command-line Tools (latest)"
        << "Android SDK Command-line Tools (latest)" << QVersionNumber({9, 0});

    QTest::newRow("cmdline-tools;8.0")
        << QStringList({"cmdline-tools;8.0",
                        "    Description:        Android SDK Command-line Tools",
                        "    Version:            8.0"})
        << "Android SDK Command-line Tools"
        << "Android SDK Command-line Tools" << QVersionNumber({8, 0});
}

void SdkManagerOutputParserTest::testParseSdkToolsPackage()
{
    QFETCH(QStringList, output);
    QFETCH(QString, description);
    QFETCH(QString, displayText);
    QFETCH(QVersionNumber, revision);

    std::unique_ptr<AndroidSdkPackage> actualSdkTool(m_parser.parseSdkToolsPackage(output));

    QVERIFY(actualSdkTool != nullptr);
    QCOMPARE(actualSdkTool->descriptionText(), description);
    QCOMPARE(actualSdkTool->displayText(), displayText);
    QCOMPARE(actualSdkTool->revision(), revision);
}

void SdkManagerOutputParserTest::testParseSdkToolsPackageEmpty()
{
    std::unique_ptr<AndroidSdkPackage> actualSdkTool(m_parser.parseSdkToolsPackage({""}));

    QVERIFY(actualSdkTool == nullptr);
}

// PlatformTools
void SdkManagerOutputParserTest::testParsePlatformToolsPackage_data()
{
    QTest::addColumn<QStringList>("output");

    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("displayText");
    QTest::addColumn<QVersionNumber>("revision");

    QTest::newRow("platform-tools")
        << QStringList({"platform-tools",
                        "    Description:        Android SDK Platform-Tools",
                        "    Version:            33.0.3"})
        << "Android SDK Platform-Tools"
        << "Android SDK Platform-Tools" << QVersionNumber({33, 0, 3});
}

void SdkManagerOutputParserTest::testParsePlatformToolsPackage()
{
    QFETCH(QStringList, output);
    QFETCH(QString, description);
    QFETCH(QString, displayText);
    QFETCH(QVersionNumber, revision);

    std::unique_ptr<AndroidSdkPackage> actualPlatformTool(
        m_parser.parsePlatformToolsPackage(output));

    QVERIFY(actualPlatformTool != nullptr);
    QCOMPARE(actualPlatformTool->descriptionText(), description);
    QCOMPARE(actualPlatformTool->displayText(), displayText);
    QCOMPARE(actualPlatformTool->revision(), revision);
}

void SdkManagerOutputParserTest::testParsePlatformToolsPackageEmpty()
{
    std::unique_ptr<AndroidSdkPackage> actualPlatformTool(m_parser.parsePlatformToolsPackage({""}));

    QVERIFY(actualPlatformTool == nullptr);
}

// EmulatorTools
void SdkManagerOutputParserTest::testParseEmulatorToolsPackage_data()
{
    QTest::addColumn<QStringList>("output");

    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("displayText");
    QTest::addColumn<QVersionNumber>("revision");

    QTest::newRow("emulator") << QStringList(
        {"emulator", "    Description:        Android Emulator", "    Version:            30.0.12"})
                              << "Android Emulator"
                              << "Android Emulator" << QVersionNumber({30, 0, 12});
}

void SdkManagerOutputParserTest::testParseEmulatorToolsPackage()
{
    QFETCH(QStringList, output);
    QFETCH(QString, description);
    QFETCH(QString, displayText);
    QFETCH(QVersionNumber, revision);

    std::unique_ptr<AndroidSdkPackage> actualEmulatorTools(
        m_parser.parseEmulatorToolsPackage(output));

    QVERIFY(actualEmulatorTools != nullptr);
    QCOMPARE(actualEmulatorTools->descriptionText(), description);
    QCOMPARE(actualEmulatorTools->displayText(), displayText);
    QCOMPARE(actualEmulatorTools->revision(), revision);
}

void SdkManagerOutputParserTest::testParseEmulatorToolsPackageEmpty()
{
    std::unique_ptr<AndroidSdkPackage> actualEmulatorTools(
        m_parser.parseEmulatorToolsPackage({""}));

    QVERIFY(actualEmulatorTools == nullptr);
}

// NDK
void SdkManagerOutputParserTest::testParseNdkPackage_data()
{
    QTest::addColumn<QStringList>("output");

    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("displayText");
    QTest::addColumn<QVersionNumber>("revision");

    QTest::newRow("ndk;21.0.6113669") << QStringList({"ndk;21.0.6113669",
                                                      "    Description:        Android NDK",
                                                      "    Version:            21.0.6113669"})
                                      << "Android NDK"
                                      << "Android NDK" << QVersionNumber({21, 0, 6113669});
}

void SdkManagerOutputParserTest::testParseNdkPackage()
{
    QFETCH(QStringList, output);
    QFETCH(QString, description);
    QFETCH(QString, displayText);
    QFETCH(QVersionNumber, revision);

    std::unique_ptr<AndroidSdkPackage> actualNdkPackage(m_parser.parseNdkPackage(output));

    QVERIFY(actualNdkPackage != nullptr);
    QCOMPARE(actualNdkPackage->descriptionText(), description);
    QCOMPARE(actualNdkPackage->displayText(), displayText);
    QCOMPARE(actualNdkPackage->revision(), revision);
}

void SdkManagerOutputParserTest::testParseNdkPackageEmpty()
{
    std::unique_ptr<AndroidSdkPackage> actualNdkPackage(m_parser.parseNdkPackage({""}));

    QVERIFY(actualNdkPackage == nullptr);
}

// ExtraTools
void SdkManagerOutputParserTest::testParseExtraToolsPackage_data()
{
    QTest::addColumn<QStringList>("output");

    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("displayText");
    QTest::addColumn<QVersionNumber>("revision");

    QTest::newRow(
        "extras;m2repository;com;android;support;constraint;constraint-layout;1.0.0-beta5")
        << QStringList(
               {"extras;m2repository;com;android;support;constraint;constraint-layout;1.0.1",
                "    Description:        ConstraintLayout for Android 1.0.1",
                "    Version:            1",
                "    Dependencies:"})
        << "ConstraintLayout for Android 1.0.1"
        << "ConstraintLayout for Android 1.0.1" << QVersionNumber({1});
}

void SdkManagerOutputParserTest::testParseExtraToolsPackage()
{
    QFETCH(QStringList, output);
    QFETCH(QString, description);
    QFETCH(QString, displayText);
    QFETCH(QVersionNumber, revision);

    std::unique_ptr<AndroidSdkPackage> actualExtraTools(m_parser.parseExtraToolsPackage(output));

    QVERIFY(actualExtraTools != nullptr);
    QCOMPARE(actualExtraTools->descriptionText(), description);
    QCOMPARE(actualExtraTools->displayText(), displayText);
    QCOMPARE(actualExtraTools->revision(), revision);
}

void SdkManagerOutputParserTest::testParseExtraToolsPackageEmpty()
{
    std::unique_ptr<AndroidSdkPackage> actualExtraTools(m_parser.parseExtraToolsPackage({""}));

    QVERIFY(actualExtraTools == nullptr);
}

// GenericTools
void SdkManagerOutputParserTest::testParseGenericToolsPackage_data()
{
    QTest::addColumn<QStringList>("output");

    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("displayText");
    QTest::addColumn<QVersionNumber>("revision");

    QTest::newRow("sources;android-33")
        << QStringList({"sources;android-33",
                        "    Description:        Sources for Android 33",
                        "    Version:            1"})
        << "Sources for Android 33"
        << "Sources for Android 33" << QVersionNumber({1});
}

void SdkManagerOutputParserTest::testParseGenericToolsPackage()
{
    QFETCH(QStringList, output);
    QFETCH(QString, description);
    QFETCH(QString, displayText);
    QFETCH(QVersionNumber, revision);

    std::unique_ptr<AndroidSdkPackage> actualGenericTools(m_parser.parseGenericTools(output));

    QVERIFY(actualGenericTools != nullptr);
    QCOMPARE(actualGenericTools->descriptionText(), description);
    QCOMPARE(actualGenericTools->displayText(), displayText);
    QCOMPARE(actualGenericTools->revision(), revision);
}

void SdkManagerOutputParserTest::testParseGenericToolsPackageEmpty()
{
    std::unique_ptr<AndroidSdkPackage> actualGenericTools(m_parser.parseGenericTools({""}));

    QVERIFY(actualGenericTools == nullptr);
}

// Platform
void SdkManagerOutputParserTest::testParsePlatformPackage_data()
{
    QTest::addColumn<QStringList>("output");

    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("installLocation");
    QTest::addColumn<QVersionNumber>("revision");
    QTest::addColumn<QString>("extension");

    QTest::newRow("platforms;android-31")
        << QStringList({"platforms;android-31",
                        "    Description:        Android SDK Platform 31",
                        "    Version:            5",
                        "    Installed Location: /home/name/Android/Sdk/platforms/android-31"})
        << "Android SDK Platform 31"
        << "/home/name/Android/Sdk/platforms/android-31" << QVersionNumber({5}) << "";

    QTest::newRow("platforms;android-33-ext4")
        << QStringList({"platforms;android-33-ext4",
                        "    Description:        Android SDK Platform 33",
                        "    Version:            1",
                        "    Installed Location: /home/name/Android/Sdk/platforms/android-33"})
        << "Android SDK Platform 33"
        << "/home/name/Android/Sdk/platforms/android-33" << QVersionNumber({1}) << " Extension 4";
}

void SdkManagerOutputParserTest::testParsePlatformPackage()
{
    QFETCH(QStringList, output);
    QFETCH(QString, description);
    QFETCH(QString, installLocation);
    QFETCH(QVersionNumber, revision);
    QFETCH(QString, extension);

    std::unique_ptr<AndroidSdkPackage> actualPlatform(m_parser.parsePlatform(output));

    QVERIFY(actualPlatform != nullptr);
    QCOMPARE(actualPlatform->descriptionText(), description);
    QCOMPARE(actualPlatform->installedLocation().path(), installLocation);
    QCOMPARE(actualPlatform->revision(), revision);
    QCOMPARE(actualPlatform->extension(), extension);
}

void SdkManagerOutputParserTest::testParsePlatformPackageEmpty()
{
    std::unique_ptr<AndroidSdkPackage> actualPlatform(m_parser.parsePlatform({""}));

    QVERIFY(actualPlatform == nullptr);
}

// SystemImage
void SdkManagerOutputParserTest::testParseSystemImagePackage_data()
{
    QTest::addColumn<QStringList>("output");

    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("installLocation");
    QTest::addColumn<QVersionNumber>("revision");

    QTest::newRow("system-images;android-31;google_apis;x86")
        << QStringList({"system-images;android-31;google_apis;x86",
                        "    Description:        Google APIs Intel x86 Atom System Image",
                        "    Version:            7",
                        "    Installed Location: /home/name/Android/Sdk/system-images/android-31/"
                                               "google_apis/x86"})
        << "Google APIs Intel x86 Atom System Image"
        << "/home/name/Android/Sdk/system-images/android-31/google_apis/x86"
        << QVersionNumber({7});
}

void SdkManagerOutputParserTest::testParseSystemImagePackage()
{
    QFETCH(QStringList, output);
    QFETCH(QString, description);
    QFETCH(QString, installLocation);
    QFETCH(QVersionNumber, revision);

    QPair<SystemImage *, int> actualSystemImagePair(m_parser.parseSystemImage(output));

    SystemImage *actualSystemImage = actualSystemImagePair.first;

    QVERIFY(actualSystemImage != nullptr);
    QCOMPARE(actualSystemImage->descriptionText(), description);
    QCOMPARE(actualSystemImage->installedLocation().path(), installLocation);
    QCOMPARE(actualSystemImage->revision(), revision);
    delete actualSystemImage;
}

void SdkManagerOutputParserTest::testParseSystemImagePackageEmpty()
{
    QPair<SystemImage *, int> actualSystemImagePair(m_parser.parseSystemImage({""}));
    SystemImage *actualSystemImage = actualSystemImagePair.first;

    QVERIFY(actualSystemImage == nullptr);
    delete actualSystemImage;
}
void SdkManagerOutputParserTest::testParsePackageListing()
{
    QFETCH(QString, sdkManagerOutput);
    QFETCH(QList<AndroidSdkPackage::PackageType>, packageTypes);
    QFETCH(int, sdkManagerOutputPackagesNumber);

    m_parser.parsePackageListing(sdkManagerOutput);

    QCOMPARE(m_packages.length(), sdkManagerOutputPackagesNumber);

    for (int i = 0; i < m_packages.length(); ++i)
        QCOMPARE(m_packages.at(i)->type(), packageTypes.at(i));
}

void SdkManagerOutputParserTest::testParsePackageListing_data()
{
    QTest::addColumn<int>("sdkManagerOutputPackagesNumber");
    QTest::addColumn<QList<AndroidSdkPackage::PackageType>>("packageTypes");
    QTest::addColumn<QString>("sdkManagerOutput");

    const QList<AndroidSdkPackage::PackageType> packageTypes = {
        AndroidSdkPackage::PackageType::BuildToolsPackage,
        AndroidSdkPackage::PackageType::SdkToolsPackage,
        AndroidSdkPackage::PackageType::EmulatorToolsPackage,
        AndroidSdkPackage::PackageType::NDKPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::SdkPlatformPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,

        AndroidSdkPackage::PackageType::BuildToolsPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::SdkToolsPackage,
        AndroidSdkPackage::PackageType::SdkToolsPackage,
        AndroidSdkPackage::PackageType::ExtraToolsPackage,
        AndroidSdkPackage::PackageType::NDKPackage,
        AndroidSdkPackage::PackageType::NDKPackage,
        AndroidSdkPackage::PackageType::PlatformToolsPackage,
        AndroidSdkPackage::PackageType::SdkPlatformPackage,
        AndroidSdkPackage::PackageType::SdkPlatformPackage,
        AndroidSdkPackage::PackageType::SdkPlatformPackage,
        AndroidSdkPackage::PackageType::SdkPlatformPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
    };

    QTest::newRow("sdkmanager --list --verbose") // version 8.0
        << 21
        << packageTypes
        << QString(R"(
Loading package information...
Loading local repository...
Info: Parsing /home/artem/Android/Sdk/build-tools/31.0.0/package.xml
Info: Parsing /home/artem/Android/Sdk/cmdline-tools/latest/package.xml
Info: Parsing /home/artem/Android/Sdk/emulator/package.xml
Info: Parsing /home/artem/Android/Sdk/ndk/21.3.6528147/package.xml
Info: Parsing /home/artem/Android/Sdk/ndk/23.1.7779620/package.xml
Info: Parsing /home/artem/Android/Sdk/ndk/25.1.8937393/package.xml
Info: Parsing /home/artem/Android/Sdk/patcher/v4/package.xml
Info: Parsing /home/artem/Android/Sdk/platform-tools/package.xml
Info: Parsing /home/artem/Android/Sdk/platforms/android-31/package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-25/google_apis/armeabi-v7a
package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-27/default/arm64-v8a/package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-29/google_apis/x86/package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-31/android-tv/arm64-v8a/package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-31/android-tv/x86/package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-31/default/x86_64/package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-31/google_apis/x86_64/package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-32/google_apis/arm64-v8a/package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-32/google_apis/x86_64/package.xml
Info: Parsing
/home/artem/Android/Sdk/system-images/android-33/google_apis/arm64-v8a/package.xml
[=========                              ] 25% Loading local repository...
[=========                              ] 25% Fetch remote repository...
[==========                             ] 26% Fetch remote repository...
[============                           ] 31% Fetch remote repository...
[=============                          ] 33% Fetch remote repository...
[=============                          ] 34% Fetch remote repository...
[==============                         ] 36% Fetch remote repository...
[==============                         ] 37% Fetch remote repository...
[===============                        ] 38% Fetch remote repository...
[===============                        ] 40% Fetch remote repository...
[================                       ] 41% Fetch remote repository...
[=================                      ] 43% Fetch remote repository...
[=================                      ] 44% Fetch remote repository...
[==================                     ] 45% Fetch remote repository...
[==================                     ] 47% Fetch remote repository...
[===================                    ] 48% Fetch remote repository...
[===================                    ] 50% Fetch remote repository...
[====================                   ] 51% Fetch remote repository...
[====================                   ] 53% Fetch remote repository...
[=====================                  ] 54% Fetch remote repository...
[======================                 ] 55% Fetch remote repository...
[======================                 ] 57% Fetch remote repository...
[=======================                ] 58% Fetch remote repository...
[=======================                ] 60% Fetch remote repository...
[========================               ] 61% Fetch remote repository...
[========================               ] 62% Fetch remote repository...
[=========================              ] 64% Fetch remote repository...
[==========================             ] 65% Fetch remote repository...
[==========================             ] 67% Fetch remote repository...
[===========================            ] 68% Fetch remote repository...
[===========================            ] 69% Fetch remote repository...
[============================           ] 71% Fetch remote repository...
[============================           ] 72% Fetch remote repository...
[=============================          ] 74% Fetch remote repository...
[=============================          ] 75% Fetch remote repository...
[=============================          ] 75% Computing updates...
[=======================================] 100% Computing updates...
Installed packages:
--------------------------------------
build-tools;31.0.0
    Description:        Android SDK Build-Tools 31
    Version:            31.0.0
    Installed Location: /home/artem/Android/Sdk/build-tools/31.0.0

cmdline-tools;latest
    Description:        Android SDK Command-line Tools (latest)
    Version:            8.0
    Installed Location: /home/artem/Android/Sdk/cmdline-tools/latest

emulator
    Description:        Android Emulator
    Version:            31.3.14
    Installed Location: /home/artem/Android/Sdk/emulator

ndk;25.1.8937393
    Description:        NDK (Side by side) 25.1.8937393
    Version:            25.1.8937393
    Installed Location: /home/artem/Android/Sdk/ndk/25.1.8937393

patcher;v4
    Description:        SDK Patch Applier v4
    Version:            1
    Installed Location: /home/artem/Android/Sdk/patcher/v4

platforms;android-31
    Description:        Android SDK Platform 31
    Version:            1
    Installed Location: /home/artem/Android/Sdk/platforms/android-31

system-images;android-33;google_apis;arm64-v8a
    Description:        Google APIs ARM 64 v8a System Image
    Version:            8
    Installed Location:
/home/artem/Android/Sdk/system-images/android-33/google_apis/arm64-v8a

Available Packages:
--------------------------------------
add-ons;addon-google_apis-google-24
    Description:        Google APIs
    Version:            1

build-tools;33.0.1
    Description:        Android SDK Build-Tools 33.0.1
    Version:            33.0.1

cmake;3.22.1
    Description:        CMake 3.22.1
    Version:            3.22.1

cmdline-tools;9.0
    Description:        Android SDK Command-line Tools
    Version:            9.0

cmdline-tools;latest
    Description:        Android SDK Command-line Tools (latest)
    Version:            9.0

emulator
    Description:        Android Emulator
    Version:            31.3.14
    Dependencies:
        patcher;v4

extras;android;m2repository
    Description:        Android Support Repository
    Version:            47.0.0

ndk-bundle
    Description:        NDK
    Version:            22.1.7171670
    Dependencies:
        patcher;v4

ndk;25.0.8775105
    Description:        NDK (Side by side) 25.0.8775105
    Version:            25.0.8775105
    Dependencies:
        patcher;v4

ndk;25.1.8937393
    Description:        NDK (Side by side) 25.1.8937393
    Version:            25.1.8937393
    Dependencies:
        patcher;v4

patcher;v4
    Description:        SDK Patch Applier v4
    Version:            1

platform-tools
    Description:        Android SDK Platform-Tools
    Version:            33.0.3

platforms;android-33
    Description:        Android SDK Platform 33
    Version:            2

platforms;android-33-ext4
    Description:        Android SDK Platform 33
    Version:            1

platforms;android-9
    Description:        Android SDK Platform 9
    Version:            2

platforms;android-TiramisuPrivacySandbox
    Description:        Android SDK Platform TiramisuPrivacySandbox
    Version:            8

sources;android-32
    Description:        Sources for Android 32
    Version:            1

sources;android-33
    Description:        Sources for Android 33
    Version:            1

system-images;android-10;default;armeabi-v7a
    Description:        ARM EABI v7a System Image
    Version:            5
    Dependencies:
        patcher;v4

system-images;android-33-ext4;google_apis_playstore;arm64-v8a
    Description:        Google Play ARM 64 v8a System Image
    Version:            1
    Dependencies:
        patcher;v4
        emulator Revision 30.7.3

system-images;android-33-ext4;google_apis_playstore;x86_64
    Description:        Google Play Intel x86 Atom_64 System Image
    Version:            1
    Dependencies:
        patcher;v4
        emulator Revision 30.7.3

system-images;android-33;android-tv;arm64-v8a
    Description:        Android TV ARM 64 v8a System Image
    Version:            5
    Dependencies:
        patcher;v4
        emulator Revision 28.1.6

system-images;android-33;android-tv;x86
    Description:        Android TV Intel x86 Atom System Image
    Version:            5
    Dependencies:
        patcher;v4
        emulator Revision 28.1.6

system-images;android-33;google-tv;arm64-v8a
    Description:        Google TV ARM 64 v8a System Image
    Version:            5
    Dependencies:
        patcher;v4
        emulator Revision 28.1.6

system-images;android-33;google-tv;x86
    Description:        Google TV Intel x86 Atom System Image
    Version:            5
    Dependencies:
        patcher;v4
        emulator Revision 28.1.6

system-images;android-33;google_apis;arm64-v8a
    Description:        Google APIs ARM 64 v8a System Image
    Version:            8
    Dependencies:
        patcher;v4
        emulator Revision 30.7.3

system-images;android-33;google_apis;x86_64
    Description:        Google APIs Intel x86 Atom_64 System Image
    Version:            8
    Dependencies:
        patcher;v4
        emulator Revision 30.7.3

system-images;android-33;google_apis_playstore;arm64-v8a
    Description:        Google Play ARM 64 v8a System Image
    Version:            7
    Dependencies:
        patcher;v4
        emulator Revision 30.7.3

system-images;android-33;google_apis_playstore;x86_64
    Description:        Google Play Intel x86 Atom_64 System Image
    Version:            7
    Dependencies:
        patcher;v4
        emulator Revision 30.7.3

system-images;android-TiramisuPrivacySandbox;google_apis_playstore;arm64-v8a
    Description:        Google Play ARM 64 v8a System Image
    Version:            8
    Dependencies:
        patcher;v4
        emulator Revision 30.7.3

system-images;android-TiramisuPrivacySandbox;google_apis_playstore;x86_64
    Description:        Google Play Intel x86 Atom_64 System Image
    Version:            8
    Dependencies:
        patcher;v4
        emulator Revision 30.7.3

Available Updates:
--------------------------------------
cmdline-tools;latest
    Installed Version: 8.0
    Available Version: 9.)");
}

void SdkManagerOutputParserTest::testParseAndroidCliPackageListing()
{
    const QString output(R"(
WARNING: The SDK Manager CLI tool (sdkmanager) is deprecated. Android CLI will be used instead.
The 'android' binary can also be found in the cmdline-tools directory, and 'android sdk' is the replacement for 'sdkmanager'.
To learn more about the Android CLI and how to use it, see the documentation (https://d.android.com/tools/agents/android-cli)

Warning: Flag --verbose is no longer supported. Ignoring.
Installed packages:
  cmdline-tools/latest                                                                    23.0.0                 Android SDK Command-line Tools (latest)
  emulator                                                                                37.1.11                Android Emulator
  extras/google/usb_driver                                                                13.0.0                 Google USB Driver
  ndk/29.0.14206865                                                                       29.0.14206865          NDK (Side by side) 29.0.14206865
  platform-tools                                                                          37.0.1                 Android SDK Platform-Tools
Available packages:
  add-ons/addon-google_apis-google-24                                                     1.0.0                  Google APIs
  add-ons/addon-google_gdk-google-19                                                      11.0.0                 Glass Development Kit Preview
  add-ons/addon-google_tv_addon-google-12                                                 2.0.0                  Google TV Addon
  add-ons/addon-google_tv_addon-google-13                                                 1.0.0                  Google TV Addon
  build-tools/37.0.0                                                                      37.0.0                 Android SDK Build-Tools 37
  build-tools/37.0.0-rc1                                                                  37.0.0-rc.1            Android SDK Build-Tools 37-rc1
  build-tools/37.0.0-rc2                                                                  37.0.0-rc.2            Android SDK Build-Tools 37-rc2
  build/lightbuild/0.0.10-alpha01                                                         0.0.10-rc.1            Lightbuild
  build/templates                                                                         0.1.1                  Android Project   (wide whitespace) Templates
  cmake/4.1.1                                                                             4.1.1                  CMake 4.1.1
  cmake/4.1.2                                                                             4.1.2                  CMake 4.1.2
  cmake/9.9.9                                                                             9.9.9
  cmdline-tools/8.0                                                                       8.0.0                  Android SDK Command-line Tools
  cmdline-tools/9.0                                                                       9.0.0                  Android SDK Command-line Tools
  cmdline-tools/latest                                                                    23.0.0                 Android SDK Command-line Tools (latest)
  emulator                                                                                37.1.11                Android Emulator
  extras/google/market_licensing                                                          1.0.0                  Google Play Licensing Library
  extras/google/simulators                                                                1.0.0                  Android Auto API Simulators
  extras/google/usb_driver                                                                13.0.0                 Google USB Driver
  extras/google/webdriver                                                                 2.0.0                  Google Web Driver
  ndk-bundle                                                                              22.1.7171670 (+13)     NDK
  ndk/30.0.14904198                                                                       30.0.14904198-rc.1     NDK (Side by side) 30.0.14904198
  ndk/30.0.15729638                                                                       30.0.15729638-rc.2     NDK (Side by side) 30.0.15729638
  ndk/30.0.16138531                                                                       30.0.16138531-rc.3     NDK (Side by side) 30.0.16138531
  platform-tools                                                                          37.0.1                 Android SDK Platform-Tools
  platforms/android-9                                                                     2.0.0                  Android SDK Platform 9
  platforms/android-CANARY                                                                15.0.0                 Android SDK Platform CANARY
  platforms/android-UpsideDownCake                                                        4.0.0                  Android SDK Platform UpsideDownCake
  skiaparser/1                                                                            6.0.0                  Layout Inspector image server for API 29-30
  skiaparser/2                                                                            3.0.0                  Layout Inspector image server for API S
  skiaparser/3                                                                            8.0.0                  Layout Inspector image server for API 31-36
  sources/android-37.0                                                                    2.0.0                  Sources for Android 37.0
  sources/android-37.1                                                                    1.0.0                  Sources for Android 37.1
  sources/android-37.2                                                                    1.0.0                  Sources for Android 37.2
  system-images/android-37.0/android-wear-signed/arm64-v8a                                1.0.0                  Wear OS 7.0 ARM 64 v8a System Image (signed)
  system-images/android-37.0/android-wear-signed/x86_64                                   1.0.0                  Wear OS 7.0 Intel x86_64 Atom System Image (signed)
  system-images/android-37.0/google_apis/arm64-v8a                                        6.0.0                  Google APIs ARM 64 v8a System Image
  system-images/android-37.0/google_apis/x86_64                                           6.0.0                  Google APIs Intel x86_64 Atom System Image
  system-images/android-37.0/google_apis_playstore/arm64-v8a                              6.0.0                  Google Play ARM 64 v8a System Image
  system-images/android-37.0/google_apis_playstore/x86_64                                 6.0.0                  Google Play Intel x86_64 Atom System Image
  system-images/android-37.0/google_apis_playstore_ps16k/arm64-v8a                        6.0.0                  16 KB Page Size Google Play ARM 64 v8a System Image
  system-images/android-37.0/google_apis_playstore_ps16k/x86_64                           6.0.0                  16 KB Page Size Google Play Intel x86_64 Atom System Image
  system-images/android-37.0/google_apis_ps16k/arm64-v8a                                  6.0.0                  16 KB Page Size Google APIs ARM 64 v8a System Image
  system-images/android-37.0/google_apis_ps16k/x86_64                                     6.0.0                  16 KB Page Size Google APIs Intel x86_64 Atom System Image
  system-images/android-37.1/google_apis_playstore_ps16k/arm64-v8a                        9.0.0                  16 KB Page Size Google Play ARM 64 v8a System Image
  system-images/android-37.1/google_apis_playstore_ps16k/x86_64                           9.0.0                  16 KB Page Size Google Play Intel x86_64 Atom System Image
  system-images/android-37.1/google_apis_ps16k/arm64-v8a                                  9.0.0                  16 KB Page Size Google APIs ARM 64 v8a System Image
  system-images/android-37.1/google_apis_ps16k/x86_64                                     9.0.0                  16 KB Page Size Google APIs Intel x86_64 Atom System Image
  system-images/android-CANARY/google_apis_ps16k/arm64-v8a                                15.0.0                 16 KB Page Size Google APIs ARM 64 v8a System Image
  system-images/android-CANARY/google_apis_ps16k/x86_64                                   15.0.0                 16 KB Page Size Google APIs Intel x86_64 Atom System Image
  tools                                                                                   26.1.1                 Android SDK Tools
)");

    const QList<AndroidSdkPackage::PackageType> expectedTypes = {
        AndroidSdkPackage::PackageType::SdkToolsPackage,
        AndroidSdkPackage::PackageType::EmulatorToolsPackage,
        AndroidSdkPackage::PackageType::ExtraToolsPackage,
        AndroidSdkPackage::PackageType::NDKPackage,
        AndroidSdkPackage::PackageType::PlatformToolsPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::BuildToolsPackage,
        AndroidSdkPackage::PackageType::BuildToolsPackage,
        AndroidSdkPackage::PackageType::BuildToolsPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::SdkToolsPackage,
        AndroidSdkPackage::PackageType::SdkToolsPackage,
        AndroidSdkPackage::PackageType::ExtraToolsPackage,
        AndroidSdkPackage::PackageType::ExtraToolsPackage,
        AndroidSdkPackage::PackageType::ExtraToolsPackage,
        AndroidSdkPackage::PackageType::NDKPackage,
        AndroidSdkPackage::PackageType::NDKPackage,
        AndroidSdkPackage::PackageType::NDKPackage,
        AndroidSdkPackage::PackageType::NDKPackage,
        AndroidSdkPackage::PackageType::SdkPlatformPackage,
        AndroidSdkPackage::PackageType::SdkPlatformPackage,
        AndroidSdkPackage::PackageType::SdkPlatformPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::GenericSdkPackage,
        AndroidSdkPackage::PackageType::SdkToolsPackage,
    };
    const int installedCount = 5;
    const Utils::FilePath sdkLocation = Utils::FilePath::fromUserInput("/location/of/Android/Sdk");

    AndroidSdkPackageList packages;
    SdkManagerOutputParser parser(packages);
    parser.parsePackageListing(output, sdkLocation);

    QCOMPARE(packages.length(), expectedTypes.length());
    for (int i = 0; i < packages.length(); ++i) {
        const AndroidSdkPackage *package = packages.at(i);
        QCOMPARE(package->type(), expectedTypes.at(i));
        const bool installed = i < installedCount;
        QCOMPARE(package->state(), installed ? AndroidSdkPackage::Installed
                                             : AndroidSdkPackage::Available);
        QCOMPARE(package->installedLocation().isEmpty(), !installed);
    }
    QCOMPARE(packages.at(3)->installedLocation(),
             sdkLocation.pathAppended("ndk/29.0.14206865"));
    qDeleteAll(packages);
}

void SdkManagerOutputParserTest::testParseVerboseListingWithoutTrailingNewline()
{
    // An output whose last line is the last detail line of the last package, without a final
    // newline, used to drop that package for the lack of a terminating empty line.
    const QString output(R"(
Installed packages:
--------------------------------------
build-tools;33.0.1
    Description:        Android SDK Build-Tools 33.0.1
    Version:            33.0.1
    Installed Location: /location/of/Android/Sdk/build-tools/33.0.1

platform-tools
    Description:        Android SDK Platform-Tools
    Version:            33.0.3
    Installed Location: /location/of/Android/Sdk/platform-tools)");

    AndroidSdkPackageList packages;
    SdkManagerOutputParser parser(packages);
    parser.parsePackageListing(output);

    QCOMPARE(packages.length(), 2);
    QCOMPARE(packages.at(0)->type(), AndroidSdkPackage::PackageType::BuildToolsPackage);
    QCOMPARE(packages.at(1)->type(), AndroidSdkPackage::PackageType::PlatformToolsPackage);
    QCOMPARE(packages.at(1)->descriptionText(), "Android SDK Platform-Tools");
    QCOMPARE(packages.at(1)->revision(), QVersionNumber(33, 0, 3));
    qDeleteAll(packages);
}

QObject *createAndroidSdkManagerOutputParserTest()
{
    return new SdkManagerOutputParserTest;
}

} // namespace Android::Internal

#include "sdkmanageroutputparser_test.moc"
