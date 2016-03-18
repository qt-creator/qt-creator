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

namespace WinRt {
namespace Internal {
namespace Constants {

const char WINRT_DEVICE_TYPE_LOCAL[] = "WinRt.Device.Local";
const char WINRT_DEVICE_TYPE_EMULATOR[] = "WinRt.Device.Emulator";
const char WINRT_DEVICE_TYPE_PHONE[] = "WinRt.Device.Phone";
const char WINRT_BUILD_STEP_DEPLOY[] = "WinRt.BuildStep.Deploy";
const char WINRT_BUILD_STEP_DEPLOY_ARGUMENTS[] = "WinRt.BuildStep.Deploy.Arguments";
const char WINRT_WINRTQT[] = "WinRt.QtVersion.WindowsRuntime";
const char WINRT_WINPHONEQT[] = "WinRt.QtVersion.WindowsPhone";
const char WINRT_QTMAP_SUBKEYNAME[] = "WinRt";
const char WINRT_QTMAP_OSFLAVOR[] = "OsFlavor";
const char WINRT_MANIFEST_EDITOR_ID[] = "WinRTManifestEditorID";
const char WINRT_RC_PREFIX[] = "WinRt.WinRtRunConfiguration:";

} // Constants
} // Internal
} // WinRt
