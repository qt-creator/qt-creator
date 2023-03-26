// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "detaildialog.h"

#include <QDialogButtonBox>
#include <QTextBrowser>
#include <QVBoxLayout>

DetailDialog::DetailDialog(QWidget *parent) :
    QDialog(parent)
{
    resize(640, 480);
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    textBrowser = new QTextBrowser(this);
    verticalLayout->addWidget(textBrowser);
    buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);

    verticalLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

DetailDialog::~DetailDialog()
{
}

void DetailDialog::setText(const QString &text)
{
    textBrowser->setPlainText(text);
}
