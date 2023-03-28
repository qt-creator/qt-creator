// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Qnx::Constants {

const char QNX_QNX_QT[] = "Qt4ProjectManager.QtVersion.QNX.QNX";
const char QNX_QNX_FEATURE[] = "QtSupport.Wizards.FeatureQNX";
const char QNX_QNX_DEPLOYCONFIGURATION_ID[] = "Qt4ProjectManager.QNX.QNXDeployConfiguration";
const char QNX_RUNCONFIG_ID[] = "Qt4ProjectManager.QNX.QNXRunConfiguration.";
const char QNX_QNX_OS_TYPE[] = "QnxOsType"; // Also used for device type.
const char QNX_TOOLCHAIN_ID[] = "Qnx.QccToolChain";

const char QNX_TMP_DIR[] = "/tmp"; // /var/run is root:root drwxr-xr-x
const char QNX_DIRECT_UPLOAD_STEP_ID[] ="Qnx.DirectUploadStep";

} // Qnx::Constants
