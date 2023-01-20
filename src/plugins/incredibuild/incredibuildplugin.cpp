// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "incredibuildplugin.h"

#include "buildconsolebuildstep.h"
#include "ibconsolebuildstep.h"

namespace IncrediBuild::Internal {

class IncrediBuildPluginPrivate
{
public:
    BuildConsoleStepFactory buildConsoleStepFactory;
    IBConsoleStepFactory iBConsoleStepFactory;
};

IncrediBuildPlugin::~IncrediBuildPlugin()
{
    delete d;
}

void IncrediBuildPlugin::initialize()
{
    d = new IncrediBuildPluginPrivate;
}

} // IncrediBuild::Internal
