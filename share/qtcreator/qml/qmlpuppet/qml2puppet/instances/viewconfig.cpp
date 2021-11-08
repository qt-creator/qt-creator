/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
