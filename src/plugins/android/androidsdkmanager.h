/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "utils/fileutils.h"
#include "androidsdkpackage.h"

#include <QObject>
#include <QFuture>

#include <memory>

namespace Android {

class AndroidConfig;

namespace Internal {

class AndroidSdkManagerPrivate;

class AndroidSdkManager : public QObject
{
    Q_OBJECT
public:
    enum CommandType
    {
        None,
        UpdateAll,
        UpdatePackage,
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

    AndroidSdkManager(const AndroidConfig &config, QObject *parent = nullptr);
    ~AndroidSdkManager();

    SdkPlatformList installedSdkPlatforms();
    const AndroidSdkPackageList &allSdkPackages();
    AndroidSdkPackageList availableSdkPackages();
    AndroidSdkPackageList installedSdkPackages();

    SdkPlatform *latestAndroidSdkPlatform(AndroidSdkPackage::PackageState state
                                          = AndroidSdkPackage::Installed);
    SdkPlatformList filteredSdkPlatforms(int minApiLevel,
                                         AndroidSdkPackage::PackageState state
                                         = AndroidSdkPackage::Installed);
    void reloadPackages(bool forceReload = false);
    bool isBusy() const;

    QFuture<QString> availableArguments() const;
    QFuture<OperationOutput> updateAll();
    QFuture<OperationOutput> update(const QStringList &install, const QStringList &uninstall);
    QFuture<OperationOutput> checkPendingLicenses();
    QFuture<OperationOutput> runLicenseCommand();

    void cancelOperatons();
    void acceptSdkLicense(bool accept);

signals:
    void packageReloadBegin();
    void packageReloadFinished();
    void cancelActiveOperations();

private:
    std::unique_ptr<AndroidSdkManagerPrivate> m_d;
    friend class AndroidSdkManagerPrivate;
};

} // namespace Internal
} // namespace Android
