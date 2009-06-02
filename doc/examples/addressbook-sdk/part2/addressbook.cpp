#include "addressbook.h"
#include "ui_addressbook.h"

AddressBook::AddressBook(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddressBookClass)
{
    ui->setupUi(this);

    addButton = new QPushButton();
    addButton = ui->addButton;

    submitButton = new QPushButton();
    submitButton = ui->submitButton;

    cancelButton = new QPushButton();
    cancelButton = ui->cancelButton;
}

AddressBook::~AddressBook()
{
    delete ui;
}

void AddressBook::addContact()
{
}

void AddressBook::submitContact()
{
}

void AddressBook::cancel()
{
}
