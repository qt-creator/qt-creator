/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#pragma once
#include "../ninjabuildstep.h"
#include "projectexplorer/buildstep.h"
#include <QListWidget>
#include <QWidget>

namespace Ui {
class MesonBuildStepConfigWidget;
}
namespace MesonProjectManager {
namespace Internal {
class MesonBuildStepConfigWidget final : public ProjectExplorer::BuildStepConfigWidget
{
    Q_DECLARE_TR_FUNCTIONS(MesonProjectManager::Internal::MesonBuildStepConfigWidget)

public:
    explicit MesonBuildStepConfigWidget(NinjaBuildStep *step);
    ~MesonBuildStepConfigWidget();

private:
    void updateDetails();
    void updateTargetList();
    inline NinjaBuildStep *mesonBuildStep() { return static_cast<NinjaBuildStep *>(step()); }
    Ui::MesonBuildStepConfigWidget *ui;
    QListWidget *m_buildTargetsList;
};

} // namespace Internal
} // namespace MesonProjectManager
