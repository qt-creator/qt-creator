// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

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
