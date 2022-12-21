// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <utils/aspects.h>

namespace ProjectExplorer { class BuildConfiguration; }

namespace QtSupport {

class QTSUPPORT_EXPORT QmlDebuggingAspect : public Utils::TriStateAspect
{
    Q_OBJECT

public:
    explicit QmlDebuggingAspect(ProjectExplorer::BuildConfiguration *buildConfig);

    void addToLayout(Utils::LayoutBuilder &builder) override;

private:
    const ProjectExplorer::BuildConfiguration *m_buildConfig = nullptr;
};

class QTSUPPORT_EXPORT QtQuickCompilerAspect : public Utils::TriStateAspect
{
    Q_OBJECT

public:
    QtQuickCompilerAspect(ProjectExplorer::BuildConfiguration *buildConfig);

private:
    void addToLayout(Utils::LayoutBuilder &builder) override;

    const ProjectExplorer::BuildConfiguration *m_buildConfig = nullptr;
};

} // namespace QtSupport
