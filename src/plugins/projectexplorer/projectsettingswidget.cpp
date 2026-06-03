// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectsettingswidget.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QLabel>

namespace ProjectExplorer {

const int CONTENTS_MARGIN = 5;

QWidget *createGlobalSettingsLink(Utils::Id globalId)
{
    const auto label = new QLabel(R"(<a href="dummy">Global settings</a>)");
    QObject::connect(label, &QLabel::linkActivated, label, [globalId] {
        Core::ICore::showSettings(globalId);
    });
    using namespace Layouting;
    return Column {
        Row { label, st, customMargins(0, CONTENTS_MARGIN, 0, CONTENTS_MARGIN), spacing(CONTENTS_MARGIN) },
        createHr(),
        noMargin, spacing(0),
    }.emerge();
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
        Row { checkBox, label, st,
              customMargins(0, CONTENTS_MARGIN, 0, CONTENTS_MARGIN),
              spacing(CONTENTS_MARGIN) },
        createHr(),
        noMargin, spacing(0),
    }.emerge();
}

} // namespace ProjectExplorer
