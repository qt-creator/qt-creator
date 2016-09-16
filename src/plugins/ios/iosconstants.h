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

#include <QtGlobal>
#include <QLoggingCategory>

namespace Ios {
namespace Internal {
Q_DECLARE_LOGGING_CATEGORY(iosLog)
} // namespace Internal

namespace Constants {
const char IOS_SETTINGS_ID[] = "CC.Ios Configurations";
const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios"; // this literal is replicated to avoid dependencies

const char IOS_DEVICE_TYPE[] = "Ios.Device.Type";
const char IOS_SIMULATOR_TYPE[] = "Ios.Simulator.Type";
const char IOS_DEVICE_ID[] = "iOS Device ";
const char IOS_SIMULATOR_DEVICE_ID[] = "iOS Simulator Device ";
const char IOS_DSYM_BUILD_STEP_ID[] = "Ios.IosDsymBuildStep";

const quint16 IOS_DEVICE_PORT_START = 30000;
const quint16 IOS_DEVICE_PORT_END = 31000;
const quint16 IOS_SIMULATOR_PORT_START = 30000;
const quint16 IOS_SIMULATOR_PORT_END = 31000;

const char EXTRA_INFO_KEY[] = "extraInfo";

} // namespace Constants;
} // namespace Ios
