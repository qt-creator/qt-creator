/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimruncontrolfactory.h"
#include "nimrunconfiguration.h"

namespace Nim {

bool NimRunControlFactory::canRun(ProjectExplorer::RunConfiguration *runConfiguration, Core::Id mode) const
{
    Q_UNUSED(mode)
    return dynamic_cast<NimRunConfiguration *>(runConfiguration);
}

ProjectExplorer::RunControl *NimRunControlFactory::create(ProjectExplorer::RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    return new ProjectExplorer::SimpleRunControl(runConfiguration, mode);
}

}

