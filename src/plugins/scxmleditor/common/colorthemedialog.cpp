// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colorthemedialog.h"
#include "colorsettings.h"

#include <utils/layoutbuilder.h>

#include <QAbstractButton>
#include <QDialogButtonBox>

using namespace ScxmlEditor::Common;

ColorThemeDialog::ColorThemeDialog(QWidget *parent)
    : QDialog(parent)
{
    resize(400, 440);

    m_colorSettings = new ColorSettings;
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                          QDialogButtonBox::Cancel |
                                          QDialogButtonBox::Apply);

    using namespace Layouting;
    Column {
        m_colorSettings,
        buttonBox,
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &ColorThemeDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ColorThemeDialog::reject);
    connect(buttonBox, &QDialogButtonBox::clicked, this, [buttonBox, this](QAbstractButton *btn) {
        if (buttonBox->standardButton(btn) == QDialogButtonBox::Apply)
            save();
    });
}

void ColorThemeDialog::save()
{
    m_colorSettings->save();
}
