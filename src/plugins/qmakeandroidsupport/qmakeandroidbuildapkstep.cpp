/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "qmakeandroidbuildapkstep.h"
#include "qmakeandroidbuildapkwidget.h"

#include <android/androidconstants.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

using namespace Android;

namespace QmakeAndroidSupport {
namespace Internal {

const Core::Id ANDROID_BUILD_APK_ID("QmakeProjectManager.AndroidBuildApkStep");


// QmakeAndroidBuildApkStepFactory

QmakeAndroidBuildApkStepFactory::QmakeAndroidBuildApkStepFactory()
{
    registerStep<QmakeAndroidBuildApkStep>(ANDROID_BUILD_APK_ID);
    setSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    setSupportedDeviceType(Constants::ANDROID_DEVICE_TYPE);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setDisplayName(QmakeAndroidBuildApkStep::tr("Build Android APK"));
    setRepeatable(false);
}

// QmakeAndroidBuildApkStep

QmakeAndroidBuildApkStep::QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc)
    : AndroidBuildApkStep(bc, ANDROID_BUILD_APK_ID)
{
}

ProjectExplorer::BuildStepConfigWidget *QmakeAndroidBuildApkStep::createConfigWidget()
{
    return new QmakeAndroidBuildApkWidget(this);
}

} // namespace Internal
} // namespace QmakeAndroidSupport
