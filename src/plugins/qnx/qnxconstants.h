/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

namespace Qnx {
namespace Constants {

const char QNX_TARGET_KEY[] = "QNX_TARGET";
const char QNX_HOST_KEY[]   = "QNX_HOST";

const char QNX_QNX_QT[] = "Qt4ProjectManager.QtVersion.QNX.QNX";

const char QNX_QNX_FEATURE[] = "QtSupport.Wizards.FeatureQNX";

const char QNX_QNX_DEPLOYCONFIGURATION_ID[] = "Qt4ProjectManager.QNX.QNXDeployConfiguration";

const char QNX_QNX_RUNCONFIGURATION_PREFIX[] = "Qt4ProjectManager.QNX.QNXRunConfiguration.";

const char QNX_QNX_OS_TYPE[] = "QnxOsType";

const char QNX_DEBUG_EXECUTABLE[] = "pdebug";

const char QNX_TOOLCHAIN_ID[] = "Qnx.QccToolChain";

// QNX settings constants
const char QNX_SETTINGS_ID[] = "DD.Qnx Configuration";

const char QNX_CONFIGS_FILENAME[] = "qnxconfigurations.xml";

const char QNX_DEBUGGING_GROUP[] = "Debugger.Group.Qnx";

} // namespace Constants
} // namespace Qnx
