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

#include <utils/filepath.h>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVersionNumber>

namespace McuSupport::Internal::Sdk {

struct PackageDescription
{
    QString label;
    QString envVar;
    QString cmakeVar;
    QString description;
    QString setting;
    Utils::FilePath defaultPath;
    Utils::FilePath validationPath;
    QList<QVersionNumber> versions;
    bool shouldAddToSystemPath;
}; //struct PackageDescription

struct McuTargetDescription
{
    enum class TargetType { MCU, Desktop };

    QString qulVersion;
    QString compatVersion;
    struct Platform
    {
        QString id;
        QString name;
        QString vendor;
        QVector<int> colorDepths;
        TargetType type;
    } platform;
    struct Toolchain
    {
        QString id;
        QStringList versions;
        QList<PackageDescription> packages;
    } toolchain;
    struct BoardSdk
    {
        QString name;
        QString defaultPath;
        QString envVar;
        QStringList versions;
        QList<PackageDescription> packages;
    } boardSdk;
    struct FreeRTOS
    {
        QString envVar;
        QString boardSdkSubDir;
        QList<PackageDescription> packages;
    } freeRTOS;
};

} // namespace McuSupport::Internal::Sdk
