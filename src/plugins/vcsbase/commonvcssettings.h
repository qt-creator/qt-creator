// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace VcsBase::Internal {

class CommonVcsSettings final : public Utils::AspectContainer
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
