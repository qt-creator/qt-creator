// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/makestep.h>

namespace QmakeProjectManager {
namespace Internal {

class QmakeMakeStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QmakeMakeStepFactory();
};

} // Internal
} // QmakeProjectManager
