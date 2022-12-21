// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addtabtotabviewdialog.h"
#include "ui_addtabtotabviewdialog.h"

namespace QmlDesigner {

AddTabToTabViewDialog::AddTabToTabViewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddTabToTabViewDialog)
{
    ui->setupUi(this);
    ui->addTabLineEdit->setForceFirstCapitalLetter(true);
}

AddTabToTabViewDialog::~AddTabToTabViewDialog()
{
    delete ui;
}


QString AddTabToTabViewDialog::create(const QString &tabName, QWidget *parent)
{
    AddTabToTabViewDialog addTabToTabViewDialog(parent);

    Utils::FileNameValidatingLineEdit *fileNameValidatingLineEdit = addTabToTabViewDialog.ui->addTabLineEdit;

    fileNameValidatingLineEdit->setText(tabName);

    int result = addTabToTabViewDialog.exec();

    if (result == QDialog::Accepted && fileNameValidatingLineEdit->isValid())
        return fileNameValidatingLineEdit->text();
    else
        return QString();
}

} // namespace QmlDesigner
