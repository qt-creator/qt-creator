// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "androidsdkpackage.h"

#include <utils/filepath.h>

#include <QObject>
#include <QFuture>

#include <memory>

QT_BEGIN_NAMESPACE
class QRegularExpression;
QT_END_MOC_NAMESPACE

namespace Android::Internal {

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
    AndroidSdkManager();
    ~AndroidSdkManager();

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

    bool packageListingSuccessful() const;

    QFuture<QString> availableArguments() const;

    void runInstallationChange(const InstallationChange &change, const QString &extraMessage = {});
    void runUpdate();

signals:
    void packageReloadBegin();
    void packageReloadFinished();

private:
    friend class AndroidSdkManagerPrivate;
    std::unique_ptr<AndroidSdkManagerPrivate> m_d;
};

const QRegularExpression &assertionRegExp();

} // namespace Android::Internal
