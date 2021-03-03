/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "analyzerrunconfigwidget.h"

#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QLayout>
#include <QPushButton>

using namespace Utils;

namespace Debugger {

AnalyzerRunConfigWidget::AnalyzerRunConfigWidget(ProjectExplorer::GlobalOrProjectAspect *aspect)
{
    using namespace Layouting;

    auto settingsCombo = new QComboBox;
    settingsCombo->addItem(tr("Global"));
    settingsCombo->addItem(tr("Custom"));

    auto restoreButton = new QPushButton(tr("Restore Global"));

    auto innerPane = new QWidget;
    auto configWidget = aspect->projectSettings()->createConfigWidget();

    auto details = new DetailsWidget;
    details->setWidget(innerPane);

    Column {
        Row { settingsCombo, restoreButton, Stretch() },
        configWidget
    }.attachTo(innerPane);

    Column { details }.attachTo(this);

    details->layout()->setContentsMargins(0, 0, 0, 0);
    innerPane->layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setContentsMargins(0, 0, 0, 0);

    auto chooseSettings = [=](int setting) {
        const bool isCustom = (setting == 1);

        settingsCombo->setCurrentIndex(setting);
        aspect->setUsingGlobalSettings(!isCustom);
        configWidget->setEnabled(isCustom);
        restoreButton->setEnabled(isCustom);
        details->setSummaryText(isCustom
                                  ? tr("Use Customized Settings")
                                  : tr("Use Global Settings"));
    };

    chooseSettings(aspect->isUsingGlobalSettings() ? 0 : 1);

    connect(settingsCombo, QOverload<int>::of(&QComboBox::activated),
            this, chooseSettings);

    connect(restoreButton, &QPushButton::clicked,
            aspect, &ProjectExplorer::GlobalOrProjectAspect::resetProjectToGlobalSettings);
}

} // namespace Debugger
