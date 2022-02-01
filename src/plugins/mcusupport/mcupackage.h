/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "mcusupportversiondetection.h"

#include <utils/filepath.h>
#include <utils/id.h>

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace ProjectExplorer {
class ToolChain;
}

namespace Utils {
class PathChooser;
class InfoLabel;
} // namespace Utils

namespace McuSupport {
namespace Internal {

class McuPackage : public QObject
{
    Q_OBJECT

public:
    enum Status {
        EmptyPath,
        InvalidPath,
        ValidPathInvalidPackage,
        ValidPackageMismatchedVersion,
        ValidPackage
    };

    McuPackage(const QString &label, const Utils::FilePath &defaultPath,
               const QString &detectionPath, const QString &settingsKey,
               const McuPackageVersionDetector *versionDetector = nullptr);
    virtual ~McuPackage() = default;

    Utils::FilePath basePath() const;
    Utils::FilePath path() const;
    QString label() const;
    Utils::FilePath defaultPath() const;
    QString detectionPath() const;
    QString statusText() const;
    void updateStatus();

    Status status() const;
    bool validStatus() const;
    void setDownloadUrl(const QString &url);
    void setEnvironmentVariableName(const QString &name);
    void setAddToPath(bool addToPath);
    bool addToPath() const;
    void writeGeneralSettings() const;
    bool writeToSettings() const;
    void setRelativePathModifier(const QString &path);
    void setVersions(const QStringList &versions);

    bool automaticKitCreationEnabled() const;
    void setAutomaticKitCreationEnabled(const bool enabled);

    QWidget *widget();

    QString environmentVariableName() const;

signals:
    void changed();
    void statusChanged();

private:
    void updatePath();
    void updateStatusUi();

    QWidget *m_widget = nullptr;
    Utils::PathChooser *m_fileChooser = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;

    const QString m_label;
    const Utils::FilePath m_defaultPath;
    const QString m_detectionPath;
    const QString m_settingsKey;
    const McuPackageVersionDetector *m_versionDetector;

    Utils::FilePath m_path;
    QString m_relativePathModifier; // relative path to m_path to be returned by path()
    QString m_detectedVersion;
    QStringList m_versions;
    QString m_downloadUrl;
    QString m_environmentVariableName;
    bool m_addToPath = false;
    bool m_automaticKitCreation = true;

    Status m_status = InvalidPath;
};

class McuToolChainPackage : public McuPackage
{
public:
    enum Type {
        TypeArmGcc,
        TypeIAR,
        TypeKEIL,
        TypeGHS,
        TypeMSVC,
        TypeGCC,
        TypeGHSArm,
        TypeUnsupported
    };

    McuToolChainPackage(const QString &label,
                        const Utils::FilePath &defaultPath,
                        const QString &detectionPath,
                        const QString &settingsKey,
                        Type type,
                        const McuPackageVersionDetector *versionDetector = nullptr
            );

    Type type() const;
    bool isDesktopToolchain() const;
    ProjectExplorer::ToolChain *toolChain(Utils::Id language) const;
    QString toolChainName() const;
    QString cmakeToolChainFileName() const;
    QVariant debuggerId() const;

private:
    const Type m_type;
};

} // namespace Internal
} // namespace McuSupport
