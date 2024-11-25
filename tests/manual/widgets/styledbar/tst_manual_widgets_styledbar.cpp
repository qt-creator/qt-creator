// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../common/themeselector.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLayout>
#include <QSpinBox>
#include <QToolButton>

#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

using namespace Utils;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto themeSelector = new ManualTest::ThemeSelector;

    StyleHelper::setToolbarStyle(StyleHelper::ToolbarStyle::Relaxed);

    const Icon icon({
        {":/utils/images/zoom.png", Theme::IconsBaseColor},
        {":/utils/images/iconoverlay_add_small.png", Theme::IconsRunColor},
    });
    const QIcon qIcon = icon.icon();

    auto styledBar = new StyledBar;

    auto toolButton = new QToolButton;
    toolButton->setCheckable(true);
    toolButton->setIcon(qIcon);

    auto toolButtonChecked = new QToolButton;
    toolButtonChecked->setCheckable(true);
    toolButtonChecked->setChecked(true);
    toolButtonChecked->setIcon(qIcon);

    auto toolButtonDisabled = new QToolButton;
    toolButtonDisabled->setEnabled(false);
    toolButtonDisabled->setIcon(qIcon);

    auto comboBox = new QComboBox;
    const QStringList content = QColor::colorNames();
    comboBox->addItems(content.first(8));

    auto spinBox = new QSpinBox;
    spinBox->setFrame(false);
    spinBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    auto checkBox = new QCheckBox("check");

    using namespace Layouting;
    Row {
        toolButton,
        toolButtonChecked,
        toolButtonDisabled,
        new Utils::StyledSeparator,
        comboBox,
        "Text label",
        spinBox,
        checkBox,
        noMargin, spacing(0),
    }.attachTo(styledBar);

    Column {
        themeSelector,
        styledBar,
        st,
    }.show();

    return app.exec();
}
