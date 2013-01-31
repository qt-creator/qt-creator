/**************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**************************************************************************/

#include "addressbook.h"
#include "ui_addressbook.h"

AddressBook::AddressBook(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddressBook)
{
    ui->setupUi(this);

    //! [setup fields]
    ui->nameLine->setReadOnly(true);
    ui->addressText->setReadOnly(true);
    ui->submitButton->hide();
    ui->cancelButton->hide();
    //! [setup fields]

    //! [signal slot]
    connect(ui->addButton, SIGNAL(clicked()), this,
                SLOT(addContact()));
    connect(ui->submitButton, SIGNAL(clicked()), this,
                SLOT(submitContact()));
    connect(ui->cancelButton, SIGNAL(clicked()), this,
                SLOT(cancel()));
    //! [signal slot]

    //! [window title]
    setWindowTitle(tr("Simple Address Book"));
    //! [window title]
}

AddressBook::~AddressBook()
{
    delete ui;
}

//! [addContact]
void AddressBook::addContact()
{
    oldName = ui->nameLine->text();
    oldAddress = ui->addressText->toPlainText();

    ui->nameLine->clear();
    ui->addressText->clear();

    ui->nameLine->setReadOnly(false);
    ui->nameLine->setFocus(Qt::OtherFocusReason);
    ui->addressText->setReadOnly(false);

    ui->addButton->setEnabled(false);
    ui->submitButton->show();
    ui->cancelButton->show();
}
//! [addContact]

//! [submitContact part1]
void AddressBook::submitContact()
{
    QString name = ui->nameLine->text();
    QString address = ui->addressText->toPlainText();

    if (name.isEmpty() || address.isEmpty()) {
        QMessageBox::information(this, tr("Empty Field"),
            tr("Please enter a name and address."));
        return;
    }
//! [submitContact part1]

//! [submitContact part2]
    if (!contacts.contains(name)) {
        contacts.insert(name, address);
        QMessageBox::information(this, tr("Add Successful"),
            tr("\"%1\" has been added to your address book.").arg(name));
    } else {
        QMessageBox::information(this, tr("Add Unsuccessful"),
            tr("Sorry, \"%1\" is already in your address book.").arg(name));
        return;
    }
//! [submitContact part2]

//! [submitContact part3]
    if (contacts.isEmpty()) {
        ui->nameLine->clear();
        ui->addressText->clear();
    }

    ui->nameLine->setReadOnly(true);
    ui->addressText->setReadOnly(true);
    ui->addButton->setEnabled(true);
    ui->submitButton->hide();
    ui->cancelButton->hide();
}
//! [submitContact part3]

//! [cancel]
void AddressBook::cancel()
{
    ui->nameLine->setText(oldName);
    ui->nameLine->setReadOnly(true);

    ui->addressText->setText(oldAddress);
    ui->addressText->setReadOnly(true);

    ui->addButton->setEnabled(true);
    ui->submitButton->hide();
    ui->cancelButton->hide();
}
//! [cancel]
