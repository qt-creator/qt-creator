/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidbuildapkstep.h"
#include "androidconstants.h"
#include "androidglobal.h"
#include "androidqtsupport.h"

#include <projectexplorer/target.h>

Utils::FileName Android::AndroidQtSupport::apkPath(ProjectExplorer::Target *target) const
{
    if (!target)
        return Utils::FileName();

    AndroidBuildApkStep *buildApkStep
        = Android::AndroidGlobal::buildStep<AndroidBuildApkStep>(target->activeBuildConfiguration());

    if (!buildApkStep)
        return Utils::FileName();

    QString apkPath;
    if (buildApkStep->useGradle())
        apkPath = QLatin1String("build/outputs/apk/android-build-");
    else
        apkPath = QLatin1String("bin/QtApp-");
    if (buildApkStep->signPackage())
        apkPath += QLatin1String("release.apk");
    else
        apkPath += QLatin1String("debug.apk");

    return target->activeBuildConfiguration()->buildDirectory()
            .appendPath(QLatin1String(Android::Constants::ANDROID_BUILDDIRECTORY))
            .appendPath(apkPath);
}
