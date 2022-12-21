// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/rawprojectpart.h>

namespace CppEditor {

// FIXME: Remove
class CppProjectUpdaterInterface
{
public:
    virtual ~CppProjectUpdaterInterface() = default;

    virtual void update(const ProjectExplorer::ProjectUpdateInfo &projectUpdateInfo) = 0;
    virtual void cancel() = 0;
};

} // namespace CppEditor
