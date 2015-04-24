/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_QNXCONSTANTS_H
#define QNX_QNXCONSTANTS_H

#include <QtGlobal>

namespace Qnx {

enum QnxArchitecture {
    X86,
    ArmLeV7,
    UnknownArch
};

namespace Constants {

const char QNX_TARGET_KEY[] = "QNX_TARGET";
const char QNX_HOST_KEY[]   = "QNX_HOST";

const char QNX_QNX_QT[] = "Qt4ProjectManager.QtVersion.QNX.QNX";

const char QNX_QNX_FEATURE[] = "QtSupport.Wizards.FeatureQNX";

const char QNX_QNX_DEPLOYCONFIGURATION_ID[] = "Qt4ProjectManager.QNX.QNXDeployConfiguration";

const char QNX_QNX_RUNCONFIGURATION_PREFIX[] = "Qt4ProjectManager.QNX.QNXRunConfiguration.";

const char QNX_QNX_OS_TYPE[] = "QnxOsType";

const char QNX_QNX_PLATFORM_NAME[] = "QNX";

const char QNX_DEBUG_EXECUTABLE[] = "pdebug";

const char QNX_TOOLCHAIN_ID[] = "Qnx.QccToolChain";

// QNX settings constants
const char QNX_CATEGORY[] = "XF.Qnx";
const char QNX_CATEGORY_TR[] = QT_TRANSLATE_NOOP("QNX", "QNX");
const char QNX_CATEGORY_ICON[] = ":/qnx/images/qnx-target.png";
const char QNX_SETTINGS_ID[] = "ZZ.Qnx Configuration";

const char QNX_CONFIGS_FILENAME[] = "qnxconfigurations.xml";

const char QNX_DEBUGGING_GROUP[] = "Debugger.Group.Qnx";

} // namespace Constants
} // namespace Qnx

#endif // QNX_QNXCONSTANTS_H
