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

private:
    void addToLayoutImpl(Layouting::Layout &parent) override;
};

class QTSUPPORT_EXPORT QtQuickCompilerAspect : public Utils::TriStateAspect
{
    Q_OBJECT

public:
    QtQuickCompilerAspect(ProjectExplorer::BuildConfiguration *buildConfig);

private:
    void addToLayoutImpl(Layouting::Layout &parent) override;
};

} // namespace QtSupport
