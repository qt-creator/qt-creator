// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectsettingswidget.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QLabel>

namespace ProjectExplorer {

QLabel *createGlobalSettingsLink(Utils::Id globalId)
{
    const auto label = new QLabel(R"(<a href="dummy">Global settings</a>)");
    QObject::connect(label, &QLabel::linkActivated, label, [globalId] {
        Core::ICore::showSettings(globalId);
    });
    return label;
}

QWidget *createGlobalOrProjectSelector(
    QObject *guard,
    bool initialUseGlobal,
    Utils::Id globalId,
    std::function<void(bool)> onChange,
    QCheckBox **checkBoxOut)
{
    const auto checkBox = new QCheckBox;
    checkBox->setChecked(initialUseGlobal);

    const auto label = new QLabel(
        QStringLiteral("Use <a href=\"dummy\">global settings</a>"));

    QObject::connect(checkBox, &QCheckBox::stateChanged,
                     guard, [onChange](int s) { onChange(s == Qt::Checked); });
    QObject::connect(label, &QLabel::linkActivated,
                     guard, [globalId] { Core::ICore::showSettings(globalId); });

    if (checkBoxOut)
        *checkBoxOut = checkBox;

    using namespace Layouting;
    return Column {
        Row { checkBox, label, st },
        createHr(),
        noMargin,
    }.emerge();
}

} // namespace ProjectExplorer
