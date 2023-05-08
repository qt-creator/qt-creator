// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace VcsBase {
namespace Internal {

class CommonVcsSettings : public Utils::AspectContainer
{
    Q_OBJECT

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

signals:
    void settingsChanged();
};

class CommonOptionsPage final : public Core::IOptionsPage
{
public:
    CommonOptionsPage();

    CommonVcsSettings &settings() { return m_settings; }

private:
    CommonVcsSettings m_settings;
};

} // namespace Internal
} // namespace VcsBase
