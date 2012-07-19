/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef ANDROIDCONSTANTS_H
#define ANDROIDCONSTANTS_H

#include <QLatin1String>

namespace Android {
namespace Internal {

enum AndroidQemuStatus {
    AndroidQemuStarting,
    AndroidQemuFailedToStart,
    AndroidQemuFinished,
    AndroidQemuCrashed,
    AndroidQemuUserReason
};

#define ANDROID_PREFIX "Qt4ProjectManager.AndroidRunConfiguration"

#ifdef Q_OS_WIN32
#define ANDROID_EXE_SUFFIX ".exe"
#define ANDROID_BAT_SUFFIX ".bat"
#else
#define ANDROID_EXE_SUFFIX ""
#define ANDROID_BAT_SUFFIX ""
#endif

static const QLatin1String ANDROID_RC_ID_PREFIX(ANDROID_PREFIX ":");

} // namespace Internal

namespace Constants {
const char ANDROID_SETTINGS_ID[] = "ZZ.Android Configurations";
const char ANDROID_SETTINGS_CATEGORY[] = "X.Android";
const char ANDROID_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Android", "Android");
const char ANDROID_SETTINGS_CATEGORY_ICON[] = ":/android/images/QtAndroid.png";
const char ANDROID_TOOLCHAIN_ID[] = "Qt4ProjectManager.ToolChain.Android";
const char ANDROIDQT[] = "Qt4ProjectManager.QtVersion.Android";

const char ANDROID_DEVICE_TYPE[] = "Android.Device.Type";
const char ANDROID_DEVICE_ID[] = "Android Device";

}
} // namespace Android

#endif  // ANDROIDCONSTANTS_H
