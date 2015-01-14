/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef IOSCONSTANTS_H
#define IOSCONSTANTS_H

#include <QtGlobal>
#include <QLoggingCategory>

namespace Ios {
namespace Internal {
Q_DECLARE_LOGGING_CATEGORY(iosLog)
} // namespace Internal

namespace Constants {
const char IOS_SETTINGS_ID[] = "ZZ.Ios Configurations";
const char IOS_SETTINGS_CATEGORY[] = "XA.Ios";
const char IOS_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Ios", "iOS");
const char IOS_SETTINGS_CATEGORY_ICON[] = ":/ios/images/iossettings.png";
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

#endif  // IOSCONSTANTS_H
