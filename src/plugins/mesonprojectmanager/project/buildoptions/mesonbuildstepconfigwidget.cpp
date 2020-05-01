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
#include "ui_mesonbuildstepconfigwidget.h"
#include <coreplugin/find/itemviewfind.h>
#include <mesonpluginconstants.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/processparameters.h>
#include <QCheckBox>
#include <QRadioButton>
namespace MesonProjectManager {
namespace Internal {
MesonBuildStepConfigWidget::MesonBuildStepConfigWidget(NinjaBuildStep *step)
    : ProjectExplorer::BuildStepConfigWidget{step}
    , ui(new Ui::MesonBuildStepConfigWidget)
    , m_buildTargetsList{new QListWidget}
{
    setDisplayName(tr("Build", "MesonProjectManager::MesonBuildStepConfigWidget display name."));
    ui->setupUi(this);
    m_buildTargetsList->setFrameStyle(QFrame::NoFrame);
    m_buildTargetsList->setMinimumHeight(200);
    ui->frame->layout()->addWidget(
        Core::ItemViewFind::createSearchableWrapper(m_buildTargetsList,
                                                    Core::ItemViewFind::LightColored));
    updateDetails();
    updateTargetList();
    connect(step, &NinjaBuildStep::commandChanged, this, &MesonBuildStepConfigWidget::updateDetails);
    connect(step,
            &NinjaBuildStep::targetListChanged,
            this,
            &MesonBuildStepConfigWidget::updateTargetList);
    connect(ui->m_toolArguments, &QLineEdit::textEdited, this, [this](const QString &text) {
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

MesonBuildStepConfigWidget::~MesonBuildStepConfigWidget()
{
    delete ui;
}

void MesonBuildStepConfigWidget::updateDetails()
{
    auto mesonBuildStep = static_cast<NinjaBuildStep *>(step());
    ProjectExplorer::ProcessParameters param;
    param.setMacroExpander(mesonBuildStep->macroExpander());
    param.setEnvironment(mesonBuildStep->buildEnvironment());
    param.setWorkingDirectory(mesonBuildStep->buildDirectory());
    param.setCommandLine(mesonBuildStep->command());
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
