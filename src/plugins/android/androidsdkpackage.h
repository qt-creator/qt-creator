/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "utils/fileutils.h"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QVersionNumber>

#pragma once

namespace Android {

namespace Internal {
    class SdkManagerOutputParser;
    class AndroidToolOutputParser;
}
class SdkPlatform;
class SystemImage;

class AndroidSdkPackage : public QObject
{
    Q_OBJECT

public:
    enum PackageType {
        UnknownPackage          = 1 << 0,
        SdkToolsPackage         = 1 << 1,
        BuildToolsPackage       = 1 << 2,
        PlatformToolsPackage    = 1 << 3,
        SdkPlatformPackage      = 1 << 4,
        SystemImagePackage      = 1 << 5,
        AnyValidType = SdkToolsPackage | BuildToolsPackage | PlatformToolsPackage |
        SdkPlatformPackage | SystemImagePackage
    };

    enum PackageState {
        Unknown     = 1 << 0,
        Installed   = 1 << 1,
        Available   = 1 << 2,
        AnyValidState = Installed | Available
    };

    AndroidSdkPackage(QVersionNumber revision, QString sdkStylePathStr, QObject *parent = nullptr);
    virtual ~AndroidSdkPackage() { }

    virtual bool isValid() const = 0;
    virtual PackageType type() const = 0;
    virtual bool operator <(const AndroidSdkPackage &other) const;

    QString displayText() const;
    QString descriptionText() const;
    const QVersionNumber &revision() const;
    PackageState state() const;
    const QString &sdkStylePath() const;
    const Utils::FileName &installedLocation() const;

protected:
    void setDisplayText(const QString &str);
    void setDescriptionText(const QString &str);
    void setState(PackageState state);
    void setInstalledLocation(const Utils::FileName &path);

    virtual void updatePackageDetails();

private:
    QString m_displayText;
    QString m_descriptionText;
    QVersionNumber m_revision;
    PackageState m_state = PackageState::Unknown;
    QString m_sdkStylePath;
    Utils::FileName m_installedLocation;

    friend class Internal::SdkManagerOutputParser;
    friend class Internal::AndroidToolOutputParser;
};
using AndroidSdkPackageList = QList<AndroidSdkPackage*>;

class SystemImage : public AndroidSdkPackage
{
    Q_OBJECT
public:
    SystemImage(QVersionNumber revision, QString sdkStylePathStr, QString abi,
                SdkPlatform *platform = nullptr);

// AndroidSdkPackage Overrides
    bool isValid() const override;
    PackageType type() const override;

    const QString &abiName() const;
    const SdkPlatform *platform() const;
    void setPlatform(SdkPlatform *platform);

private:
    QPointer<SdkPlatform> m_platform;
    QString m_abiName;
};
using SystemImageList = QList<SystemImage*>;


class SdkPlatform : public AndroidSdkPackage
{
    Q_OBJECT
public:
    SdkPlatform(QVersionNumber revision, QString sdkStylePathStr, int api,
                QObject *parent = nullptr);

    ~SdkPlatform();

// AndroidSdkPackage Overrides
    bool isValid() const override;
    PackageType type() const override;
    bool operator <(const AndroidSdkPackage &other) const override;

    int apiLevel() const;
    QVersionNumber version() const;
    void addSystemImage(SystemImage *image);
    SystemImageList systemImages(AndroidSdkPackage::PackageState state
                                 = AndroidSdkPackage::Installed) const;

private:
    SystemImageList m_systemImages;
    int m_apiLevel = -1;
    QVersionNumber m_version;
};
using SdkPlatformList = QList<SdkPlatform*>;

class BuildTools : public AndroidSdkPackage
{
public:
    BuildTools(QVersionNumber revision, QString sdkStylePathStr, QObject *parent = nullptr);

// AndroidSdkPackage Overrides
public:
    bool isValid() const override;
    PackageType type() const override;
};

class PlatformTools : public AndroidSdkPackage
{
public:
    PlatformTools(QVersionNumber revision, QString sdkStylePathStr, QObject *parent = nullptr);

// AndroidSdkPackage Overrides
public:
    bool isValid() const override;
    PackageType type() const override;
};

class SdkTools : public AndroidSdkPackage
{
public:
    SdkTools(QVersionNumber revision, QString sdkStylePathStr, QObject *parent = nullptr);

// AndroidSdkPackage Overrides
public:
    bool isValid() const override;
    PackageType type() const override;
};
} // namespace Android


