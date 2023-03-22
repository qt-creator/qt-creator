// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace Android::Internal {

class AndroidDeployQtStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    AndroidDeployQtStepFactory();
};

} // Android::Internal
