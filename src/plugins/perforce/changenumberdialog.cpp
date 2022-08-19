// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "changenumberdialog.h"

#include <QIntValidator>

using namespace Perforce::Internal;

ChangeNumberDialog::ChangeNumberDialog(QWidget *parent) : QDialog(parent)
{
    m_ui.setupUi(this);
    m_ui.numberLineEdit->setValidator(new QIntValidator(0, 1000000, this));
}

int ChangeNumberDialog::number() const
{
    if (m_ui.numberLineEdit->text().isEmpty())
        return -1;
    bool ok;
    return m_ui.numberLineEdit->text().toInt(&ok);
}
