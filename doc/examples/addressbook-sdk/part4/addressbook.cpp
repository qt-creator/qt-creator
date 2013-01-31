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

    ui->nameLine->setReadOnly(true);
    ui->addressText->setReadOnly(true);
    ui->submitButton->hide();
    ui->cancelButton->hide();
    ui->nextButton->setEnabled(false);
    ui->previousButton->setEnabled(false);

//! [extract objects]
    ui->editButton->setEnabled(false);
    ui->removeButton->setEnabled(false);
//! [extract objects]

    connect(ui->addButton, SIGNAL(clicked()), this,
                SLOT(addContact()));
    connect(ui->submitButton, SIGNAL(clicked()), this,
                SLOT(submitContact()));
    connect(ui->cancelButton, SIGNAL(clicked()), this,
                SLOT(cancel()));
    connect(ui->nextButton, SIGNAL(clicked()), this,
                SLOT(next()));
    connect(ui->previousButton, SIGNAL(clicked()), this,
                SLOT(previous()));
//! [signal slot]
    connect(ui->editButton, SIGNAL(clicked()), this,
                SLOT(editContact()));
    connect(ui->removeButton, SIGNAL(clicked()), this,
                SLOT(removeContact()));
//! [signal slot]

    setWindowTitle(tr("Simple Address Book"));
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

    updateInterface(AddingMode);
}
//! [addContact]

//! [submitContact part1]
void AddressBook::submitContact()
{
//! [submitContact part1]
    QString name = ui->nameLine->text();
    QString address = ui->addressText->toPlainText();

    if (name.isEmpty() || address.isEmpty()) {
        QMessageBox::information(this, tr("Empty Field"),
            tr("Please enter a name and address."));
    }

//! [submitContact part2]
    if (currentMode == AddingMode) {

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
    } else if (currentMode == EditingMode) {

        if (oldName != name) {
            if (!contacts.contains(name)) {
                QMessageBox::information(this, tr("Edit Successful"),
                    tr("\"%1\" has been edited in your address book.").arg(oldName));
                contacts.remove(oldName);
                contacts.insert(name, address);
            } else  {
                QMessageBox::information(this, tr("Edit Unsuccessful"),
                    tr("Sorry, \"%1\" is already in your address book.").arg(name));
                return;
            }
        } else if (oldAddress != address) {
            QMessageBox::information(this, tr("Edit Successful"),
                tr("\"%1\" has been edited in your address book.").arg(name));
            contacts[name] = address;
        }
    }
    updateInterface(NavigationMode);
}
//! [submitContact part3]

//! [cancel]
void AddressBook::cancel()
{
    ui->nameLine->setText(oldName);
    ui->nameLine->setReadOnly(true);

    updateInterface(NavigationMode);
}
//! [cancel]

void AddressBook::next()
{
    QString name = ui->nameLine->text();
    QMap<QString, QString>::iterator i = contacts.find(name);

    if (i != contacts.end())
        i++;
    if (i == contacts.end())
        i = contacts.begin();

    ui->nameLine->setText(i.key());
    ui->addressText->setText(i.value());
}

void AddressBook::previous()
{
    QString name = ui->nameLine->text();
    QMap<QString, QString>::iterator i = contacts.find(name);

    if (i == contacts.end()) {
        ui->nameLine->clear();
        ui->addressText->clear();
        return;
    }

    if (i == contacts.begin())
        i = contacts.end();

    i--;
    ui->nameLine->setText(i.key());
    ui->addressText->setText(i.value());
}

//! [editContact]
void AddressBook::editContact()
{
    oldName = ui->nameLine->text();
    oldAddress = ui->addressText->toPlainText();

    updateInterface(EditingMode);
}
//! [editContact]

//! [removeContact]
void AddressBook::removeContact()
{
    QString name = ui->nameLine->text();
    QString address = ui->addressText->toPlainText();

    if (contacts.contains(name)) {
        int button = QMessageBox::question(this,
            tr("Confirm Remove"),
            tr("Are you sure you want to remove \"%1\"?").arg(name),
            QMessageBox::Yes | QMessageBox::No);

        if (button == QMessageBox::Yes) {
            previous();
            contacts.remove(name);

            QMessageBox::information(this, tr("Remove Successful"),
                tr("\"%1\" has been removed from your address book.").arg(name));
        }
    }

    updateInterface(NavigationMode);
}
//! [removeContact]

//! [updateInterface part1]
void AddressBook::updateInterface(Mode mode)
{
    currentMode = mode;

    switch (currentMode) {

    case AddingMode:
    case EditingMode:

        ui->nameLine->setReadOnly(false);
        ui->nameLine->setFocus(Qt::OtherFocusReason);
        ui->addressText->setReadOnly(false);

        ui->addButton->setEnabled(false);
        ui->editButton->setEnabled(false);
        ui->removeButton->setEnabled(false);

        ui->nextButton->setEnabled(false);
        ui->previousButton->setEnabled(false);

        ui->submitButton->show();
        ui->cancelButton->show();
        break;
//! [updateInterface part1]

//! [updateInterface part2]
    case NavigationMode:

        if (contacts.isEmpty()) {
            ui->nameLine->clear();
            ui->addressText->clear();
        }

        ui->nameLine->setReadOnly(true);
        ui->addressText->setReadOnly(true);
        ui->addButton->setEnabled(true);

        int number = contacts.size();
        ui->editButton->setEnabled(number >= 1);
        ui->removeButton->setEnabled(number >= 1);
        ui->nextButton->setEnabled(number > 1);
        ui->previousButton->setEnabled(number >1);

        ui->submitButton->hide();
        ui->cancelButton->hide();
        break;
    }
}
//! [updateInterface part2]
