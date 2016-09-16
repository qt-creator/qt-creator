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

#pragma once

#include <QtGlobal>

namespace Android {
namespace Internal {

enum AndroidQemuStatus {
    AndroidQemuStarting,
    AndroidQemuFailedToStart,
    AndroidQemuFinished,
    AndroidQemuCrashed,
    AndroidQemuUserReason
};

#ifdef Q_OS_WIN32
#define ANDROID_BAT_SUFFIX ".bat"
#else
#define ANDROID_BAT_SUFFIX ""
#endif

} // namespace Internal

namespace Constants {
const char ANDROID_SETTINGS_ID[] = "BB.Android Configurations";
const char ANDROID_TOOLCHAIN_ID[] = "Qt4ProjectManager.ToolChain.Android";
const char ANDROIDQT[] = "Qt4ProjectManager.QtVersion.Android";

const char ANDROID_DEVICE_TYPE[] = "Android.Device.Type";
const char ANDROID_DEVICE_ID[] = "Android Device";
const char ANDROID_MANIFEST_MIME_TYPE[] = "application/vnd.google.android.android_manifest";
const char ANDROID_MANIFEST_EDITOR_ID[] = "Android.AndroidManifestEditor.Id";
const char ANDROID_MANIFEST_EDITOR_CONTEXT[] = "Android.AndroidManifestEditor.Id";

const char ANDROID_BUILDDIRECTORY[] = "android-build";
const char JAVA_EDITOR_ID[] = "java.editor";
const char JAVA_MIMETYPE[] = "text/x-java";
const char ANDROID_ARCHITECTURE[] = "Android.Architecture";
const char ANDROID_DEPLOY_SETTINGS_FILE[] = "AndroidDeploySettingsFile";
const char ANDROID_PACKAGE_SOURCE_DIR[] = "AndroidPackageSourceDir";
const char ANDROID_EXTRA_LIBS[] = "AndroidExtraLibs";

} // namespace Constants;
} // namespace Android
