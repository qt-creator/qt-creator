// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprojectnodes.h"

#include <utils/fsengine/fileiconprovider.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>

using namespace ProjectExplorer;

namespace QmlProjectManager::Internal {

QmlProjectNode::QmlProjectNode(Project *project)
    : ProjectNode(project->projectDirectory())
{
    setDisplayName(project->projectFilePath().completeBaseName());

    setIcon(DirectoryIcon(":/projectexplorer/images/fileoverlay_qml.png"));
}

} // namespace QmlProjectManager
