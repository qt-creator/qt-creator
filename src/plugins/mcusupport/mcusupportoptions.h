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

#include <utils/id.h>

#include <QObject>
#include <QVector>
#include <QVersionNumber>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Utils {
class FilePath;
class PathChooser;
class InfoLabel;
}

namespace ProjectExplorer {
class Kit;
class ToolChain;
}

namespace McuSupport {
namespace Internal {

class McuPackage : public QObject
{
    Q_OBJECT

public:
    enum Status {
        InvalidPath,
        ValidPathInvalidPackage,
        ValidPackage
    };

    McuPackage(const QString &label, const QString &defaultPath, const QString &detectionPath,
               const QString &settingsKey);
    virtual ~McuPackage() = default;

    QString path() const;
    QString label() const;
    QString defaultPath() const;
    QString detectionPath() const;
    Status status() const;
    void setDownloadUrl(const QString &url);
    void setEnvironmentVariableName(const QString &name);
    void setAddToPath(bool addToPath);
    bool addToPath() const;
    void writeToSettings() const;
    void setRelativePathModifier(const QString &path);

    QWidget *widget();

    QString environmentVariableName() const;

signals:
    void changed();

private:
    void updateStatus();

    QWidget *m_widget = nullptr;
    Utils::PathChooser *m_fileChooser = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;

    const QString m_label;
    const QString m_defaultPath;
    const QString m_detectionPath;
    const QString m_settingsKey;

    QString m_path;
    QString m_relativePathModifier; // relative path to m_path to be returned by path()
    QString m_downloadUrl;
    QString m_environmentVariableName;
    bool m_addToPath = false;

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
        TypeDesktop
    };

    McuToolChainPackage(const QString &label, const QString &defaultPath,
                        const QString &detectionPath, const QString &settingsKey, Type type);

    Type type() const;
    ProjectExplorer::ToolChain *toolChain(Utils::Id language) const;
    QString cmakeToolChainFileName() const;
    QVariant debuggerId() const;

private:
    const Type m_type;
};

class McuTarget : public QObject
{
    Q_OBJECT

public:
    enum class OS {
        Desktop,
        BareMetal,
        FreeRTOS
    };

    McuTarget(const QVersionNumber &qulVersion, const QString &vendor, const QString &platform,
              OS os, const QVector<McuPackage *> &packages,
              const McuToolChainPackage *toolChainPackage);

    QVersionNumber qulVersion() const;
    QString vendor() const;
    QVector<McuPackage *> packages() const;
    const McuToolChainPackage *toolChainPackage() const;
    QString qulPlatform() const;
    OS os() const;
    void setColorDepth(int colorDepth);
    int colorDepth() const;
    bool isValid() const;

private:
    const QVersionNumber m_qulVersion;
    const QString m_vendor;
    const QString m_qulPlatform;
    const OS m_os = OS::BareMetal;
    const QVector<McuPackage*> m_packages;
    const McuToolChainPackage *m_toolChainPackage;
    int m_colorDepth = -1;
};

class McuSupportOptions : public QObject
{
    Q_OBJECT

public:
    McuSupportOptions(QObject *parent = nullptr);
    ~McuSupportOptions() override;

    QVector<McuPackage*> packages;
    QVector<McuTarget*> mcuTargets;
    McuPackage *qtForMCUsSdkPackage = nullptr;

    void setQulDir(const Utils::FilePath &dir);
    static Utils::FilePath qulDirFromSettings();

    static QString kitName(const McuTarget* mcuTarget);

    static QList<ProjectExplorer::Kit *> existingKits(const McuTarget *mcuTarget);
    static QList<ProjectExplorer::Kit *> outdatedKits();
    static void removeOutdatedKits();
    static ProjectExplorer::Kit *newKit(const McuTarget *mcuTarget, const McuPackage *qtForMCUsSdk);
    void populatePackagesAndTargets();
    static void registerQchFiles();
    static void registerExamples();

    static const QVersionNumber &minimalQulVersion();

private:
    void deletePackagesAndTargets();

signals:
    void changed();
};

} // namespace Internal
} // namespace McuSupport
