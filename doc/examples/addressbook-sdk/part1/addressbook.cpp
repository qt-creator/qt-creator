//! [class implementation]
#include "addressbook.h"
#include "ui_addressbook.h"

AddressBook::AddressBook(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddressBookClass)
{
    ui->setupUi(this);
}

AddressBook::~AddressBook()
{
    delete ui;
}
//! [class implementation]
