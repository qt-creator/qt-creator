// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ieditorfactory.h"

namespace Core::Internal {

class SystemEditor : public IExternalEditor
{
public:
    explicit SystemEditor();

    bool startEditor(const Utils::FilePath &filePath, QString *errorMessage) override;
};

} // namespace Core::Internal
