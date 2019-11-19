/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include <QObject>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace Utils {
class PathChooser;
}

namespace ProjectExplorer {
class Kit;
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

    QString path() const;
    QString label() const;
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
    QLabel *m_statusIcon = nullptr;
    QLabel *m_statusLabel = nullptr;

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

class McuTarget : public QObject
{
    Q_OBJECT

public:
    McuTarget(const QString &vendor, const QString &model, const QVector<McuPackage *> &packages);

    QString vendor() const;
    QString model() const;
    QVector<McuPackage *> packages() const;
    void setToolChainFile(const QString &toolChainFile);
    QString toolChainFile() const;
    void setQulPlatform(const QString &qulPlatform);
    QString qulPlatform() const;
    void setColorDepth(int colorDepth);
    int colorDepth() const;
    bool isValid() const;

private:
    const QString m_vendor;
    const QString m_model;
    const QVector<McuPackage*> m_packages;
    QString m_toolChainFile;
    QString m_qulPlatform;
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
    McuPackage *armGccPackage = nullptr;
    McuPackage *qtForMCUsSdkPackage = nullptr;

    QString kitName(const McuTarget* mcuTarget) const;

    QList<ProjectExplorer::Kit *> existingKits(const McuTarget *mcuTargt);
    ProjectExplorer::Kit *newKit(const McuTarget *mcuTarget);

signals:
    void changed();
};

} // namespace Internal
} // namespace McuSupport
