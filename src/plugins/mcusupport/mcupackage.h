// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcuabstractpackage.h"
#include "mcusupportversiondetection.h"
#include "settingshandler.h"

#include <utils/filepath.h>
#include <utils/pathchooser.h>

#include <QObject>

namespace ProjectExplorer {
class Toolchain;
}

namespace Utils {
class InfoLabel;
class Id;
} // namespace Utils

namespace McuSupport::Internal {

class McuPackage : public McuAbstractPackage
{
    Q_OBJECT

public:
    McuPackage(const SettingsHandler::Ptr &settingsHandler,
               const QString &label,
               const Utils::FilePath &defaultPath,
               const Utils::FilePaths &detectionPaths,
               const Utils::Key &settingsKey,
               const QString &cmakeVarName,
               const QString &envVarName,
               const QStringList &versions = {},
               const QString &downloadUrl = {},
               const McuPackageVersionDetector *versionDetector = nullptr,
               const bool addToPath = false,
               const Utils::PathChooser::Kind &valueType
               = Utils::PathChooser::Kind::ExistingDirectory,
               const bool allowNewerVersionKey = false);

    ~McuPackage() override = default;

    static const QMap<QString, QString> packageLabelTranslations;

    QString label() const override;
    QString cmakeVariableName() const override;
    QString environmentVariableName() const override;
    bool isAddToSystemPath() const override;
    QStringList versions() const override;

    Utils::FilePath basePath() const override;
    Utils::FilePath path() const override;
    Utils::FilePath defaultPath() const override;
    Utils::FilePaths detectionPaths() const override;
    QString detectionPathsToString() const override;
    Utils::Key settingsKey() const final;

    void updateStatus() override;
    Status status() const override;
    bool isValidStatus() const override;
    QString statusText() const override;

    bool writeToSettings() const override;
    void readFromSettings() override;

    QWidget *widget() override;
    const McuPackageVersionDetector *getVersionDetector() const override;

    void setPath(const Utils::FilePath &) override;

private:
    void updatePath();
    void updateStatusUi();

    SettingsHandler::Ptr settingsHandler;

    Utils::PathChooser *m_fileChooser = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;

    const QString m_label;
    Utils::FilePath m_defaultPath;
    const Utils::FilePaths m_detectionPaths;
    Utils::FilePath m_usedDetectionPath;
    const Utils::Key m_settingsKey;
    QScopedPointer<const McuPackageVersionDetector> m_versionDetector;

    Utils::FilePath m_path;
    QString m_detectedVersion;
    QStringList m_versions;
    const QString m_cmakeVariableName;
    const QString m_environmentVariableName;
    const QString m_downloadUrl;
    const bool m_addToSystemPath;
    const Utils::PathChooser::Kind m_valueType;

    Status m_status = Status::InvalidPath;
}; // class McuPackage

class McuToolchainPackage final : public McuPackage
{
    Q_OBJECT
public:
    enum class ToolchainType { IAR, KEIL, MSVC, GCC, ArmGcc, GHS, GHSArm, MinGW, Unsupported };

    McuToolchainPackage(const SettingsHandler::Ptr &settingsHandler,
                        const QString &label,
                        const Utils::FilePath &defaultPath,
                        const Utils::FilePaths &detectionPaths,
                        const Utils::Key &settingsKey,
                        ToolchainType toolchainType,
                        const QStringList &versions,
                        const QString &cmakeVarName,
                        const QString &envVarName,
                        const McuPackageVersionDetector *versionDetector);

    ToolchainType toolchainType() const;
    bool isDesktopToolchain() const;
    ProjectExplorer::Toolchain *toolChain(Utils::Id language) const;
    QString toolchainName() const;
    QVariant debuggerId() const;

    static ProjectExplorer::Toolchain *msvcToolchain(Utils::Id language);
    static ProjectExplorer::Toolchain *gccToolchain(Utils::Id language);

private:
    const ToolchainType m_type;
};

} // namespace McuSupport::Internal

Q_DECLARE_METATYPE(McuSupport::Internal::McuToolchainPackage::ToolchainType)
