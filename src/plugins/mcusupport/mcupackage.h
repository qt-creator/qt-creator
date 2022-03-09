/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#pragma once

#include "mcuabstractpackage.h"

#include <utils/filepath.h>

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace ProjectExplorer {
class ToolChain;
}

namespace Utils {
class PathChooser;
class InfoLabel;
class Id;
} // namespace Utils

namespace McuSupport {
namespace Internal {

class McuPackageVersionDetector;

class McuPackage : public McuAbstractPackage
{
    Q_OBJECT

public:
    McuPackage(const QString &label,
               const Utils::FilePath &defaultPath,
               const Utils::FilePath &detectionPath,
               const QString &settingsKey,
               const QString &cmakeVarName,
               const QString &envVarName,
               const QString &downloadUrl = {},
               const McuPackageVersionDetector *versionDetector = nullptr,
               const bool addToPath = false,
               const Utils::FilePath &relativePathModifier = Utils::FilePath());

    QString label() const override;
    const QString &cmakeVariableName() const override;
    const QString &environmentVariableName() const override;
    bool isAddToSystemPath() const override;
    void setVersions(const QStringList &versions) override;

    Utils::FilePath basePath() const override;
    Utils::FilePath path() const override;
    Utils::FilePath defaultPath() const override;
    Utils::FilePath detectionPath() const override;
    QString settingsKey() const final;

    void updateStatus() override;
    Status status() const override;
    bool isValidStatus() const override;
    QString statusText() const override;

    bool writeToSettings() const override;

    QWidget *widget() override;

private:
    void updatePath();
    void updateStatusUi();

    QWidget *m_widget = nullptr;
    Utils::PathChooser *m_fileChooser = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;

    const QString m_label;
    const Utils::FilePath m_defaultPath;
    const Utils::FilePath m_detectionPath;
    const QString m_settingsKey;
    const McuPackageVersionDetector *m_versionDetector;

    Utils::FilePath m_path;
    Utils::FilePath m_relativePathModifier; // relative path to m_path to be returned by path()
    QString m_detectedVersion;
    QStringList m_versions;
    const QString m_cmakeVariableName;
    const QString m_environmentVariableName;
    const QString m_downloadUrl;
    const bool m_addToSystemPath;

    Status m_status = Status::InvalidPath;
};

class McuToolChainPackage : public McuPackage
{
public:
    enum class ToolChainType { IAR, KEIL, MSVC, GCC, ArmGcc, GHS, GHSArm, Unsupported };

    McuToolChainPackage(const QString &label,
                        const Utils::FilePath &defaultPath,
                        const Utils::FilePath &detectionPath,
                        const QString &settingsKey,
                        ToolChainType toolchainType,
                        const QString &cmakeVarName = {},
                        const QString &envVarName = {},
                        const McuPackageVersionDetector *versionDetector = nullptr);

    ToolChainType toolchainType() const;
    bool isDesktopToolchain() const;
    ProjectExplorer::ToolChain *toolChain(Utils::Id language) const;
    QString toolChainName() const;
    QString cmakeToolChainFileName() const;
    QVariant debuggerId() const;

private:
    const ToolChainType m_type;
};

} // namespace Internal
} // namespace McuSupport
