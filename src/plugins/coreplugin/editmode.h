// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imode.h"

namespace Core::Internal {

class EditMode final : public IMode
{
public:
    EditMode();
    ~EditMode() final;

private:
    void grabEditorManager(Utils::Id mode);
};

} // namespace Core::Internal
