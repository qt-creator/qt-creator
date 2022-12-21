// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectnodes.h>

namespace QmlProjectManager {
namespace Internal {

class QmlProjectNode : public ProjectExplorer::ProjectNode
{
public:
    explicit QmlProjectNode(ProjectExplorer::Project *project);
};

} // namespace Internal
} // namespace QmlProjectManager
