// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectnodes.h>

#include <QString>

namespace Utils { class FilePath; }

namespace QbsProjectManager::Internal {

class QbsProject;

void runStepsForNamedProducts(QbsProject *project, const QStringList &products,
                              ProjectExplorer::BuildAction action);
void buildSingleFile(QbsProject *project, const Utils::FilePath &file);

} // QbsProjectManager::Private
