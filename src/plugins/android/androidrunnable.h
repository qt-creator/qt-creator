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

#include "android_global.h"
#include <projectexplorer/runnables.h>

namespace Android {

struct ANDROID_EXPORT AndroidRunnable
{
    AndroidRunnable();
    QString packageName;
    QString intentName;
    QStringList amStartExtraArgs;
    Utils::Environment environment;
    QStringList beforeStartAdbCommands;
    QStringList afterFinishAdbCommands;
    QString deviceSerialNumber;

    QString displayName() const { return packageName; }
    static void *staticTypeId;
};

inline bool operator==(const AndroidRunnable &r1, const AndroidRunnable &r2)
{
    return r1.packageName == r2.packageName
        && r1.intentName == r2.intentName
        && r1.amStartExtraArgs == r2.amStartExtraArgs
        && r1.environment == r2.environment
        && r1.beforeStartAdbCommands == r2.beforeStartAdbCommands
        && r1.afterFinishAdbCommands == r2.afterFinishAdbCommands
        && r1.deviceSerialNumber == r2.deviceSerialNumber;
}

inline bool operator!=(const AndroidRunnable &r1, const AndroidRunnable &r2)
{
    return !(r1 == r2);
}

} // namespace Android
Q_DECLARE_METATYPE(Android::AndroidRunnable)
