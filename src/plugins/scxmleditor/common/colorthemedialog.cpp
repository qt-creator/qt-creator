// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "colorthemedialog.h"

using namespace ScxmlEditor::Common;

ColorThemeDialog::ColorThemeDialog(QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);

    connect(m_ui.m_btnOk, &QPushButton::clicked, this, &ColorThemeDialog::accept);
    connect(m_ui.m_btnCancel, &QPushButton::clicked, this, &ColorThemeDialog::reject);
    connect(m_ui.m_btnApply, &QPushButton::clicked, this, &ColorThemeDialog::save);
}

void ColorThemeDialog::save()
{
    m_ui.m_colorSettings->save();
}
