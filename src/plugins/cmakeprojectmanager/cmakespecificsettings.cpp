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

#include "cmakespecificsettings.h"

namespace CMakeProjectManager {
namespace Internal {

namespace {
static const char SETTINGS_KEY[] = "CMakeSpecificSettings";
static const char AFTER_ADD_FILE_ACTION_KEY[] = "ProjectPopupSetting";
static const char NINJA_PATH[] = "NinjaPath";
}

void CMakeSpecificSettings::fromSettings(QSettings *settings)
{
    const QString rootKey = QString(SETTINGS_KEY) + '/';
    m_afterAddFileToProjectSetting = static_cast<AfterAddFileAction>(
                              settings->value(rootKey + AFTER_ADD_FILE_ACTION_KEY,
                                              static_cast<int>(AfterAddFileAction::ASK_USER)).toInt());

    m_ninjaPath = Utils::FilePath::fromUserInput(
        settings->value(rootKey + NINJA_PATH, QString()).toString());
}

void CMakeSpecificSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QString(SETTINGS_KEY));
    settings->setValue(QString(AFTER_ADD_FILE_ACTION_KEY), static_cast<int>(m_afterAddFileToProjectSetting));
    settings->endGroup();
}
}
}
