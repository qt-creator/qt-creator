// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include "androidsdkpackage.h"

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class SdkManagerOutputParser;

class SdkManagerOutputParserTest : public QObject
{
    Q_OBJECT
public:
    SdkManagerOutputParserTest(QObject *parent = nullptr);
    ~SdkManagerOutputParserTest();

private:
    AndroidSdkPackageList m_packages;
    std::unique_ptr<SdkManagerOutputParser> m_parser;

private slots:
    void testParsePackageListing_data();
    void testParsePackageListing();

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
} // namespace Internal
} // namespace Android
