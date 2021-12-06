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

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>
#include <utils/qtcsettings.h>

namespace VcsBase {
namespace Internal {

class CommonVcsSettings : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(VcsBase::Internal::CommonVcsSettings)

public:
    CommonVcsSettings();

    friend QDebug operator<<(QDebug, const CommonVcsSettings &);

    Utils::StringAspect nickNameMailMap;
    Utils::StringAspect nickNameFieldListFile;

    Utils::StringAspect submitMessageCheckScript;

    // Executable run to graphically prompt for a SSH-password.
    Utils::StringAspect sshPasswordPrompt;

    Utils::BoolAspect lineWrap;
    Utils::IntegerAspect lineWrapWidth;
};

class CommonOptionsPage final : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CommonOptionsPage();

    CommonVcsSettings &settings() { return m_settings; }

signals:
    void settingsChanged();

private:
    CommonVcsSettings m_settings;
};

} // namespace Internal
} // namespace VcsBase
