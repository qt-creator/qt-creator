// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "analyzerrunconfigwidget.h"

#include "../debuggertr.h"

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
    settingsCombo->addItem(Tr::tr("Global"));
    settingsCombo->addItem(Tr::tr("Custom"));

    auto restoreButton = new QPushButton(Tr::tr("Restore Global"));

    auto innerPane = new QWidget;
    auto configWidget = aspect->projectSettings()->layouter()().emerge();

    auto details = new DetailsWidget;
    details->setWidget(innerPane);

    Column {
        Row { settingsCombo, restoreButton, st },
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
                                  ? Tr::tr("Use Customized Settings")
                                  : Tr::tr("Use Global Settings"));
    };

    chooseSettings(aspect->isUsingGlobalSettings() ? 0 : 1);

    connect(settingsCombo, &QComboBox::activated, this, chooseSettings);
    connect(restoreButton, &QPushButton::clicked,
            aspect, &ProjectExplorer::GlobalOrProjectAspect::resetProjectToGlobalSettings);
}

} // namespace Debugger
