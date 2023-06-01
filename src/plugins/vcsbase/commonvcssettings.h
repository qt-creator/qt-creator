// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/iversioncontrol.h>

namespace VcsBase::Internal {

class CommonVcsSettings : public Core::PagedSettings
{
public:
    CommonVcsSettings();

    Utils::FilePathAspect nickNameMailMap{this};
    Utils::FilePathAspect nickNameFieldListFile{this};

    Utils::FilePathAspect submitMessageCheckScript{this};

    // Executable run to graphically prompt for a SSH-password.
    Utils::FilePathAspect sshPasswordPrompt{this};

    Utils::BoolAspect lineWrap{this};
    Utils::IntegerAspect lineWrapWidth{this};
};

CommonVcsSettings &commonSettings();

} // VcsBase::Internal
