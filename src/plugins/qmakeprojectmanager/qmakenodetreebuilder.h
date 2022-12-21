// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"
#include "qmakeparsernodes.h"
#include "qmakenodes.h"
#include "qmakeproject.h"

namespace QmakeProjectManager {

class QmakeNodeTreeBuilder
{
public:
    static std::unique_ptr<QmakeProFileNode> buildTree(QmakeBuildSystem *buildSystem);
};

} // namespace QmakeProjectManager
