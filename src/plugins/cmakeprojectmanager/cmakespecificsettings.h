/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <utils/fileutils.h>

#include <QSettings>

namespace CMakeProjectManager {
namespace Internal {

enum AfterAddFileAction : int {
    ASK_USER,
    COPY_FILE_PATH,
    NEVER_COPY_FILE_PATH
};

class CMakeSpecificSettings
{
public:
    CMakeSpecificSettings() = default;
    void fromSettings(QSettings *settings);
    void toSettings(QSettings *settings) const;

    void setAfterAddFileSetting(AfterAddFileAction settings) { m_afterAddFileToProjectSetting = settings; }
    AfterAddFileAction afterAddFileSetting() const { return m_afterAddFileToProjectSetting; }

    Utils::FilePath ninjaPath() const { return m_ninjaPath; }

private:
    AfterAddFileAction m_afterAddFileToProjectSetting;
    Utils::FilePath m_ninjaPath;
};

}
}
