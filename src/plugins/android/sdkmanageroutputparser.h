// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "androidsdkpackage.h"

#include <utils/filepath.h>

#include <QVersionNumber>

namespace Android {
namespace Internal {
/*!
    \class SdkManagerOutputParser
    \brief The SdkManagerOutputParser class is a helper class to parse the output of the \c sdkmanager
    commands.
 */
class SdkManagerOutputParser
{
    class GenericPackageData
    {
    public:
        bool isValid() const { return !revision.isNull() && !description.isNull(); }
        QStringList headerParts;
        QVersionNumber revision;
        QString description;
        Utils::FilePath installedLocation;
        QMap<QString, QString> extraData;
    };

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

    AndroidSdkPackageList &m_packages;

private:
    void compilePackageAssociations();
    void parsePackageData(MarkerTag packageMarker, const QStringList &data);
    bool parseAbstractData(GenericPackageData &output, const QStringList &input, int minParts,
                           const QString &logStrTag,
                           const QStringList &extraKeys = QStringList()) const;
    AndroidSdkPackage *parsePlatform(const QStringList &data) const;
    QPair<SystemImage *, int> parseSystemImage(const QStringList &data) const;
    BuildTools *parseBuildToolsPackage(const QStringList &data) const;
    SdkTools *parseSdkToolsPackage(const QStringList &data) const;
    PlatformTools *parsePlatformToolsPackage(const QStringList &data) const;
    EmulatorTools *parseEmulatorToolsPackage(const QStringList &data) const;
    Ndk *parseNdkPackage(const QStringList &data) const;
    ExtraTools *parseExtraToolsPackage(const QStringList &data) const;
    GenericSdkPackage *parseGenericTools(const QStringList &data) const;
    MarkerTag parseMarkers(const QString &line);

    MarkerTag m_currentSection = MarkerTag::None;
    QHash<AndroidSdkPackage *, int> m_systemImages;
    friend class SdkManagerOutputParserTest;
};
} // namespace Internal
} // namespace Android
