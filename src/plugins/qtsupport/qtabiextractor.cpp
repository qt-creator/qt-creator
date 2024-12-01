// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtabiextractor.h"

#include "baseqtversion.h"
#include "qtsupporttr.h"

#include <coreplugin/messagemanager.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QVersionNumber>

#include <utility>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport::Internal {
Q_LOGGING_CATEGORY(abiDetect, "qtc.qtsupport.detectAbis", QtWarningMsg)

class QtAbiExtractor
{
public:
    QtAbiExtractor(const QVersionNumber &qtVersion, const FilePath &jsonFile)
        : m_qtVersion(qtVersion), m_jsonFile(jsonFile) {}

    Abis getAbis()
    {
        const auto printErrorAndReturn = [this](const QString &detail) {
            printError(detail);
            return Abis();
        };
        const auto content = m_jsonFile.fileContents();
        if (!content)
            return printErrorAndReturn(content.error());

        QJsonParseError parseError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(content.value(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
            return printErrorAndReturn(parseError.errorString());

        m_mainObject = jsonDoc.object();
        if (m_qtVersion < QVersionNumber(6, 9))
            extractAbisV1();
        else
            extractAbisV2();
        return m_abis;
    }

private:
    void extractAbisV1()
    {
        const QJsonObject obj = m_mainObject.value("built_with").toObject();
        const QString osString = obj.value("target_system").toString();
        const Abi::OS os = getOs(osString);

        if (os == Abi::DarwinOS)
            return; // QTBUG-129996

        const auto [arch, width] = getArch(obj.value("architecture").toString());
        if (os == Abi::UnknownOS && arch != Abi::AsmJsArchitecture)
            return printError(Tr::tr("Could not determine target OS"));
        if (arch == Abi::UnknownArchitecture)
            return printError(Tr::tr("Could not determine target architecture"));

        const Abi::OSFlavor flavor
            = getFlavor(os, osString, getCompilerId(obj), getCompilerVersion(obj));
        if (flavor == Abi::UnknownFlavor && arch != Abi::AsmJsArchitecture)
            return printError(Tr::tr("Could not determine OS sub-type"));

        m_abis.emplaceBack(arch, os, flavor, getFormat(os, arch), width);
    }

    void extractAbisV2()
    {
        for (const QJsonArray platforms = m_mainObject.value("platforms").toArray();
             const QJsonValue &p : platforms) {
            const QJsonObject platform = p.toObject();
            const QString osString = platform.value("name").toString();
            const Abi::OS os = getOs(osString);
            const QString compilerId = getCompilerId(platform);
            const QString compilerVersion = getCompilerVersion(platform);
            for (const QJsonArray &targets = platform.value("targets").toArray();
                 const QJsonValue &t : targets) {
                const QJsonObject target = t.toObject();
                const auto [arch, width] = getArch(target.value("architecture").toString());
                if (os == Abi::UnknownOS && arch != Abi::AsmJsArchitecture)
                    return printError(Tr::tr("Could not determine target OS"));
                if (arch == Abi::UnknownArchitecture)
                    return printError(Tr::tr("Could not determine target architecture"));

                const Abi::OSFlavor flavor = getFlavor(os, osString, compilerId, compilerVersion);
                if (flavor == Abi::UnknownFlavor && arch != Abi::AsmJsArchitecture)
                    return printError(Tr::tr("Could not determine OS sub-type"));

                m_abis.emplaceBack(arch, os, flavor, getFormat(os, arch), width);
            }
        }
    }

    void printError(const QString &detail) {
        Core::MessageManager::writeSilently(
            Tr::tr("Error reading \"%1\": %2").arg(m_jsonFile.toUserOutput(), detail));
    }

    Abi::OS getOs(const QString &osString) const
    {
        if (osString == "Linux" || osString == "Android")
            return Abi::LinuxOS;
        if (osString == "Darwin" || osString == "iOS")
            return Abi::DarwinOS;
        if (osString == "Windows")
            return Abi::WindowsOS;
        if (osString.endsWith("BSD"))
            return Abi::BsdOS;
        return Abi::UnknownOS;
    }

    std::pair<Abi::Architecture, int> getArch(const QString &archString)
    {
        if (archString == "x86" || archString == "i386")
            return std::make_pair(Abi::X86Architecture, 32);
        if (archString == "x86_64")
            return std::make_pair(Abi::X86Architecture, 64);
        if (archString == "arm")
            return std::make_pair(Abi::ArmArchitecture, 32);
        if (archString == "arm64")
            return std::make_pair(Abi::ArmArchitecture, 64);
        if (archString == "riscv64")
            return std::make_pair(Abi::RiscVArchitecture, 64);
        if (archString == "wasm")
            return std::make_pair(Abi::AsmJsArchitecture, 32);
        return std::make_pair(Abi::UnknownArchitecture, 0);
    }

    Abi::OSFlavor getFlavor(
        Abi::OS os,
        const QString &osString,
        const QString &compilerId,
        const QString &compilerVersion)
    {
        if (osString == "Android")
            return Abi::AndroidLinuxFlavor;
        switch (os) {
        case Abi::LinuxOS:
        case Abi::DarwinOS:
        case Abi::BareMetalOS:
        case Abi::QnxOS:
            return Abi::GenericFlavor;
        case Abi::BsdOS:
            if (osString == "FreeBSD")
                return Abi::FreeBsdFlavor;
            if (osString == "NetBSD")
                return Abi::NetBsdFlavor;
            if (osString == "OpenBSD")
                return Abi::OpenBsdFlavor;
            break;
        case Abi::WindowsOS: {
            if (compilerId == "GNU" || compilerId == "Clang")
                return Abi::WindowsMSysFlavor;

            // https://learn.microsoft.com/en-us/cpp/overview/compiler-versions
            const QVersionNumber rawVersion = QVersionNumber::fromString(compilerVersion);
            switch (rawVersion.majorVersion()) {
            case 19:
                if (rawVersion.minorVersion() >= 30)
                    return Abi::WindowsMsvc2022Flavor;
                if (rawVersion.minorVersion() >= 20)
                    return Abi::WindowsMsvc2019Flavor;
                if (rawVersion.minorVersion() >= 10)
                    return Abi::WindowsMsvc2017Flavor;
                return Abi::WindowsMsvc2015Flavor;
            case 18:
                return Abi::WindowsMsvc2013Flavor;
            case 17:
                return Abi::WindowsMsvc2012Flavor;
            case 16:
                return Abi::WindowsMsvc2010Flavor;
            case 15:
                return Abi::WindowsMsvc2008Flavor;
            case 14:
                return Abi::WindowsMsvc2005Flavor;
            }
            break;
        }
        case Abi::UnixOS:
        case Abi::VxWorks:
        case Abi::UnknownOS:
            break;
        }
        return Abi::UnknownFlavor;
    }

    Abi::BinaryFormat getFormat(Abi::OS os, Abi::Architecture arch)
    {
        if (arch == Abi::AsmJsArchitecture)
            return Abi::EmscriptenFormat;
        switch (os) {
        case Abi::BareMetalOS:
        case Abi::BsdOS:
        case Abi::LinuxOS:
        case Abi::QnxOS:
        case Abi::UnixOS:
        case Abi::VxWorks:
            return Abi::ElfFormat;
        case Abi::DarwinOS:
            return Abi::MachOFormat;
        case Abi::WindowsOS:
            return Abi::PEFormat;
        case Abi::UnknownOS:
            break;
        }
        return Abi::UnknownFormat;
    }

    QString getCompilerId(const QJsonObject &obj) const
    {
        return obj.value("compiler_id").toString();
    }

    QString getCompilerVersion(const QJsonObject &obj) const
    {
        return obj.value("compiler_version").toString();
    }

    const QVersionNumber m_qtVersion;
    const FilePath m_jsonFile;
    QJsonObject m_mainObject;
    Abis m_abis;
};

Abis qtAbisFromJson(const QtVersion &qtVersion, const Utils::FilePaths &possibleLocations)
{
    if (qtVersion.qtVersion().majorVersion() < 6) {
        qCDebug(abiDetect) << "Not attempting to read JSON file for Qt < 6";
        return {};
    }

    FilePath jsonFile;
    for (const FilePath &baseDir : possibleLocations) {
        const FilePath candidate = baseDir.pathAppended("modules/Core.json");
        qCDebug(abiDetect) << "Checking JSON file location" << candidate;
        if (candidate.exists()) {
            jsonFile = candidate;
            break;
        }
    }
    if (jsonFile.isEmpty()) {
        Core::MessageManager::writeSilently(
            Tr::tr("Core.json not found for Qt at \"%1\"")
                .arg(qtVersion.qmakeFilePath().toUserOutput()));
        return {};
    }

    return QtAbiExtractor(qtVersion.qtVersion(), jsonFile).getAbis();
}

} // namespace QtSupport::Internal
