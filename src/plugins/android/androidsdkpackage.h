// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <utils/fileutils.h>

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
        EmulatorToolsPackage    = 1 << 6,
        NDKPackage              = 1 << 7,
        ExtraToolsPackage       = 1 << 8,
        GenericSdkPackage       = 1 << 9,
        AnyValidType = SdkToolsPackage | BuildToolsPackage | PlatformToolsPackage |
        SdkPlatformPackage | SystemImagePackage | EmulatorToolsPackage | NDKPackage |
                       ExtraToolsPackage | GenericSdkPackage
    };

    enum PackageState {
        Unknown     = 1 << 0,
        Installed   = 1 << 1,
        Available   = 1 << 2,
        AnyValidState = Installed | Available
    };

    AndroidSdkPackage(const QVersionNumber &revision, const QString &sdkStylePathStr,
                      QObject *parent = nullptr);
    ~AndroidSdkPackage() override = default;

    virtual bool isValid() const = 0;
    virtual PackageType type() const = 0;
    virtual bool operator <(const AndroidSdkPackage &other) const;

    QString displayText() const;
    QString descriptionText() const;
    QString extension() const;
    const QVersionNumber &revision() const;
    PackageState state() const;
    const QString &sdkStylePath() const;
    const Utils::FilePath &installedLocation() const;

protected:
    void setDisplayText(const QString &str);
    void setDescriptionText(const QString &str);
    void setState(PackageState state);
    void setInstalledLocation(const Utils::FilePath &path);
    void setExtension(const QString &extension);

    virtual void updatePackageDetails();

private:
    QString m_displayText;
    QString m_descriptionText;
    QVersionNumber m_revision;
    PackageState m_state = PackageState::Unknown;
    QString m_sdkStylePath;
    QString m_extension;
    Utils::FilePath m_installedLocation;

    friend class Internal::SdkManagerOutputParser;
    friend class Internal::AndroidToolOutputParser;
};
using AndroidSdkPackageList = QList<AndroidSdkPackage*>;

class SystemImage : public AndroidSdkPackage
{
    Q_OBJECT
public:
    SystemImage(const QVersionNumber &revision, const QString &sdkStylePathStr, const QString &abi,
                SdkPlatform *platform = nullptr);

// AndroidSdkPackage Overrides
    bool isValid() const override;
    PackageType type() const override;

    const QString &abiName() const;
    const SdkPlatform *platform() const;
    void setPlatform(SdkPlatform *platform);
    int apiLevel() const;
    void setApiLevel(const int apiLevel);

private:
    QPointer<SdkPlatform> m_platform;
    QString m_abiName;
    int m_apiLevel = -1;
};
using SystemImageList = QList<SystemImage*>;


class SdkPlatform : public AndroidSdkPackage
{
    Q_OBJECT
public:
    SdkPlatform(const QVersionNumber &revision, const QString &sdkStylePathStr, int api,
                QObject *parent = nullptr);

    ~SdkPlatform() override;

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
    BuildTools(const QVersionNumber &revision, const QString &sdkStylePathStr,
               QObject *parent = nullptr);

// AndroidSdkPackage Overrides
public:
    bool isValid() const override;
    PackageType type() const override;
};
using BuildToolsList = QList<BuildTools*>;

class PlatformTools : public AndroidSdkPackage
{
public:
    PlatformTools(const QVersionNumber &revision, const QString &sdkStylePathStr,
                  QObject *parent = nullptr);

// AndroidSdkPackage Overrides
public:
    bool isValid() const override;
    PackageType type() const override;
};

class EmulatorTools : public AndroidSdkPackage
{
public:
    EmulatorTools(const QVersionNumber &revision, const QString &sdkStylePathStr,
                  QObject *parent = nullptr);

// AndroidSdkPackage Overrides
public:
    bool isValid() const override;
    PackageType type() const override;
};

class SdkTools : public AndroidSdkPackage
{
public:
    SdkTools(const QVersionNumber &revision, const QString &sdkStylePathStr,
             QObject *parent = nullptr);

// AndroidSdkPackage Overrides
public:
    bool isValid() const override;
    PackageType type() const override;
};

class Ndk : public AndroidSdkPackage
{
public:
    Ndk(const QVersionNumber &revision, const QString &sdkStylePathStr, QObject *parent = nullptr);

    // AndroidSdkPackage Overrides
    bool isValid() const override;
    PackageType type() const override;
};
using NdkList = QList<Ndk *>;

class ExtraTools : public AndroidSdkPackage
{
public:
    ExtraTools(const QVersionNumber &revision, const QString &sdkStylePathStr,
               QObject *parent = nullptr);

// AndroidSdkPackage Overrides
public:
    bool isValid() const override;
    PackageType type() const override;
};

class GenericSdkPackage : public AndroidSdkPackage
{
public:
    GenericSdkPackage(const QVersionNumber &revision, const QString &sdkStylePathStr,
                      QObject *parent = nullptr);

// AndroidSdkPackage Overrides
public:
    bool isValid() const override;
    PackageType type() const override;
};
} // namespace Android


