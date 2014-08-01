/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef WINRTCONSTANTS_H
#define WINRTCONSTANTS_H

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

#endif // WINRTCONSTANTS_H
