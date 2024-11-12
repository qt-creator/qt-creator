// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "androidsdkpackage.h"

namespace Android::Internal {
/*!
    \class SdkManagerOutputParser
    \brief The SdkManagerOutputParser class is a helper class to parse the output of the \c sdkmanager
    commands.
 */
class SdkManagerOutputParser
{
public:
    enum MarkerTag
    {
        None                        = 0x001,
        InstalledPackagesMarker     = 0x002,
        AvailablePackagesMarkers    = 0x004,
        AvailableUpdatesMarker      = 0x008,
        EmptyMarker                 = 0x010,
        PlatformMarker              = 0x020,
        SystemImageMarker           = 0x040,
        BuildToolsMarker            = 0x080,
        SdkToolsMarker              = 0x100,
        PlatformToolsMarker         = 0x200,
        EmulatorToolsMarker         = 0x400,
        NdkMarker                   = 0x800,
        ExtrasMarker                = 0x1000,
        CmdlineSdkToolsMarker       = 0x2000,
        GenericToolMarker           = 0x4000,
        SectionMarkers = InstalledPackagesMarker | AvailablePackagesMarkers | AvailableUpdatesMarker
    };

    SdkManagerOutputParser(AndroidSdkPackageList &container) : m_packages(container) {}
    void parsePackageListing(const QString &output);

private:
    void compilePackageAssociations();
    void parsePackageData(MarkerTag packageMarker, const QStringList &data);
    AndroidSdkPackage *parsePlatform(const QStringList &data) const;
    QPair<SystemImage *, int> parseSystemImage(const QStringList &data) const;
    AndroidSdkPackage *parseBuildToolsPackage(const QStringList &data) const;
    AndroidSdkPackage *parseSdkToolsPackage(const QStringList &data) const;
    AndroidSdkPackage *parsePlatformToolsPackage(const QStringList &data) const;
    AndroidSdkPackage *parseEmulatorToolsPackage(const QStringList &data) const;
    AndroidSdkPackage *parseNdkPackage(const QStringList &data) const;
    AndroidSdkPackage *parseExtraToolsPackage(const QStringList &data) const;
    AndroidSdkPackage *parseGenericTools(const QStringList &data) const;
    MarkerTag parseMarkers(const QString &line);

    AndroidSdkPackageList &m_packages;
    MarkerTag m_currentSection = MarkerTag::None;
    QHash<AndroidSdkPackage *, int> m_systemImages;
    friend class SdkManagerOutputParserTest;
};

} // namespace Android::Internal
