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
#include "mesonbuildstepconfigwidget.h"
#include <mesonpluginconstants.h>
#include <coreplugin/find/itemviewfind.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/processparameters.h>
#include <QFormLayout>
#include <QRadioButton>

namespace MesonProjectManager {
namespace Internal {

MesonBuildStepConfigWidget::MesonBuildStepConfigWidget(NinjaBuildStep *step)
    : ProjectExplorer::BuildStepConfigWidget{step}
    , m_buildTargetsList{new QListWidget}
{
    setDisplayName(tr("Build", "MesonProjectManager::MesonBuildStepConfigWidget display name."));

    m_toolArguments = new QLineEdit(this);

    m_buildTargetsList->setMinimumHeight(200);
    m_buildTargetsList->setFrameShape(QFrame::StyledPanel);
    m_buildTargetsList->setFrameShadow(QFrame::Raised);

    auto wrapper = Core::ItemViewFind::createSearchableWrapper(m_buildTargetsList,
                                                               Core::ItemViewFind::LightColored);

    auto formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->addRow(tr("Tool arguments:"), m_toolArguments);
    formLayout->addRow(tr("Targets:"), wrapper);

    updateDetails();
    updateTargetList();
    connect(step, &NinjaBuildStep::commandChanged, this, &MesonBuildStepConfigWidget::updateDetails);
    connect(step,
            &NinjaBuildStep::targetListChanged,
            this,
            &MesonBuildStepConfigWidget::updateTargetList);
    connect(m_toolArguments, &QLineEdit::textEdited, this, [this](const QString &text) {
        auto mesonBuildStep = static_cast<NinjaBuildStep *>(this->step());
        mesonBuildStep->setCommandArgs(text);
        updateDetails();
    });
    connect(m_buildTargetsList, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
        if (item->checkState() == Qt::Checked) {
            mesonBuildStep()->setBuildTarget(item->data(Qt::UserRole).toString());
            updateDetails();
        }
    });
}

void MesonBuildStepConfigWidget::updateDetails()
{
    auto mesonBuildStep = static_cast<NinjaBuildStep *>(step());
    ProjectExplorer::ProcessParameters param;
    mesonBuildStep->setupProcessParameters(&param);
    setSummaryText(param.summary(displayName()));
}

void MesonBuildStepConfigWidget::updateTargetList()
{
    m_buildTargetsList->clear();
    for (const auto &target : mesonBuildStep()->projectTargets()) {
        auto item = new QListWidgetItem(m_buildTargetsList);
        auto button = new QRadioButton(target);
        connect(button, &QRadioButton::toggled, this, [this, target](bool toggled) {
            if (toggled) {
                mesonBuildStep()->setBuildTarget(target);
                updateDetails();
            }
        });
        button->setChecked(mesonBuildStep()->targetName() == target);
        m_buildTargetsList->setItemWidget(item, button);
        item->setData(Qt::UserRole, target);
    }
}

} // namespace Internal
} // namespace MesonProjectManager
