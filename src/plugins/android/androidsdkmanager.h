// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "androidsdkpackage.h"

#include <utils/fileutils.h>

#include <QObject>
#include <QFuture>

#include <memory>

namespace Android {

class AndroidConfig;

namespace Internal {

class AndroidSdkManagerPrivate;

struct InstallationChange
{
    QStringList toInstall;
    QStringList toUninstall = {};
    int count() const { return toInstall.count() + toUninstall.count(); }
};

class AndroidSdkManager : public QObject
{
    Q_OBJECT
public:
    enum CommandType
    {
        None,
        UpdateInstalled,
        UpdatePackages,
        LicenseCheck,
        LicenseWorkflow
    };

    struct OperationOutput
    {
        bool success = false;
        CommandType type = None;
        QString stdOutput;
        QString stdError;
    };

    AndroidSdkManager();
    ~AndroidSdkManager() override;

    SdkPlatformList installedSdkPlatforms();
    const AndroidSdkPackageList &allSdkPackages();
    QStringList notFoundEssentialSdkPackages();
    QStringList missingEssentialSdkPackages();
    AndroidSdkPackageList installedSdkPackages();
    SystemImageList installedSystemImages();
    NdkList installedNdkPackages();

    SdkPlatform *latestAndroidSdkPlatform(AndroidSdkPackage::PackageState state
                                          = AndroidSdkPackage::Installed);
    SdkPlatformList filteredSdkPlatforms(int minApiLevel,
                                         AndroidSdkPackage::PackageState state
                                         = AndroidSdkPackage::Installed);
    BuildToolsList filteredBuildTools(int minApiLevel,
                                      AndroidSdkPackage::PackageState state
                                      = AndroidSdkPackage::Installed);
    void refreshPackages();
    void reloadPackages();
    bool isBusy() const;

    bool packageListingSuccessful() const;

    QFuture<QString> availableArguments() const;
    QFuture<OperationOutput> updateInstalled();
    QFuture<OperationOutput> updatePackages(const InstallationChange &change);
    QFuture<OperationOutput> licenseCheck();
    QFuture<OperationOutput> licenseWorkflow();

    void cancelOperatons();
    void acceptSdkLicense(bool accept);

    void runInstallationChange(const InstallationChange &change, const QString &extraMessage = {});
    void runUpdate();

signals:
    void packageReloadBegin();
    void packageReloadFinished();
    void cancelActiveOperations();

private:
    friend class AndroidSdkManagerPrivate;
    std::unique_ptr<AndroidSdkManagerPrivate> m_d;
};

int parseProgress(const QString &out, bool &foundAssertion);
} // namespace Internal
} // namespace Android
