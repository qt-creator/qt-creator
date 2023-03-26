// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "viewconfig.h"
#include <QtGlobal>

bool ViewConfig::isQuick3DMode()
{
    static bool mode3D = qEnvironmentVariableIsSet("QMLDESIGNER_QUICK3D_MODE");
    return mode3D;
}

static bool particleViewEnabled = false;
void ViewConfig::enableParticleView(bool enable)
{
    particleViewEnabled = enable;
}

bool ViewConfig::isParticleViewMode()
{
    static bool particleviewmode = !qEnvironmentVariableIsSet("QT_QUICK3D_DISABLE_PARTICLE_SYSTEMS");
    return particleviewmode && particleViewEnabled;
}
