/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

const int QNX_BB_QT_FACTORY_PRIO  = 60;
const int QNX_QNX_QT_FACTORY_PRIO = 50;

const char QNX_TARGET_KEY[] = "QNX_TARGET";
const char QNX_HOST_KEY[]   = "QNX_HOST";

const char QNX_BB_QT[]  = "Qt4ProjectManager.QtVersion.QNX.BlackBerry";
const char QNX_QNX_QT[] = "Qt4ProjectManager.QtVersion.QNX.QNX";

const char QNX_BB_FEATURE[]  = "QtSupport.Wizards.FeatureBlackBerry";
const char QNX_QNX_FEATURE[] = "QtSupport.Wizards.FeatureQNX";

const char QNX_BB_X86_TARGET_ID[]     = "Qt4ProjectManager.Target.QNX.BBX86Target";
const char QNX_BB_ARMLEV7_TARGET_ID[] = "Qt4ProjectManager.Target.QNX.BBArmLeV7Target";
const char QNX_QNX_X86_TARGET_ID[]     = "Qt4ProjectManager.Target.QNX.QNXX86Target";
const char QNX_QNX_ARMLEV7_TARGET_ID[] = "Qt4ProjectManager.Target.QNX.QNXArmLeV7Target";

const char QNX_BB_DEPLOYCONFIGURATION_ID[]     = "Qt4ProjectManager.QNX.BBDeployConfiguration";
const char QNX_QNX_DEPLOYCONFIGURATION_ID[]     = "Qt4ProjectManager.QNX.QNXDeployConfiguration";

const char QNX_BB_RUNCONFIGURATION_PREFIX[] = "Qt4ProjectManager.QNX.BBRunConfiguration.";
const char QNX_QNX_RUNCONFIGURATION_PREFIX[] = "Qt4ProjectManager.QNX.QNXRunConfiguration.";

const char QNX_CREATE_PACKAGE_BS_ID[] = "Qt4ProjectManager.QnxCreatePackageBuildStep";
const char QNX_DEPLOY_PACKAGE_BS_ID[] = "Qt4ProjectManager.QnxDeployPackageBuildStep";

const char QNX_PROFILEPATH_KEY[]   = "Qt4ProjectManager.QnxRunConfiguration.ProFilePath";

const char QNX_BB_OS_TYPE[]  = "BBOsType";
const char QNX_QNX_OS_TYPE[] = "QnxOsType";

const char QNX_DEBUG_TOKEN_KEY[] = "debugToken";

const char QNX_BLACKBERRY_CASCADES_WIZARD_ID[]     = "Q.QnxBlackBerryCascadesApp";
const char QNX_BAR_DESCRIPTOR_WIZARD_ID[]          = "Q.QnxBlackBerryBarDescriptor";
const char QNX_BLACKBERRY_QTQUICK_APP_WIZARD_ID[]  = "Q.QnxBlackBerryQQApp";
const char QNX_BLACKBERRY_QTQUICK2_APP_WIZARD_ID[] = "Q.QnxBlackBerryQQ2App";
const char QNX_BLACKBERRY_GUI_APP_WIZARD_ID[]      = "Q.QnxBlackBerryGuiApp";

const char QNX_QNX_PLATFORM_NAME[] = "QNX";
const char QNX_BB_PLATFORM_NAME[]  = "BlackBerry";

const char QNX_DEBUG_EXECUTABLE[] = "pdebug";

// BlackBerry settings constants
const char QNX_BB_CATEGORY[] = "XF.BlackBerry";
const char QNX_BB_CATEGORY_TR[] = QT_TRANSLATE_NOOP("BlackBerry", "BlackBerry");
const char QNX_BB_CATEGORY_ICON[] = ":/qnx/images/target.png";
const char QNX_BB_NDK_SETTINGS_ID[] = "ZZ.BlackBerry NDK Configuration";

const char QNX_BAR_DESCRIPTOR_MIME_TYPE[] = "application/vnd.rim.qnx.bar_descriptor";
const char QNX_BAR_DESCRIPTOR_EDITOR_ID[] = "Qnx.BarDescriptorEditor";

const char QNX_TASK_CATEGORY_BARDESCRIPTOR[] = "Task.Category.BarDescriptor";
} // namespace Constants
} // namespace Qnx

#endif // QNX_QNXCONSTANTS_H
