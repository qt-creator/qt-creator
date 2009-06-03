#include "addressbook.h"
#include "ui_addressbook.h"

AddressBook::AddressBook(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddressBookClass)
{
    ui->setupUi(this);

    //! [extract objects]
    nameLine = new QLineEdit;
    nameLine = ui->nameLine;
    nameLine->setReadOnly(true);

    addressText = new QTextEdit;
    addressText = ui->addressText;
    addressText->setReadOnly(true);

    addButton = new QPushButton;
    addButton = ui->addButton;

    submitButton = new QPushButton;
    submitButton = ui->submitButton;
    submitButton->hide();

    cancelButton = new QPushButton;
    cancelButton = ui->cancelButton;
    cancelButton->hide();
    //! [extract objects]

    //! [signal slot]
    connect(addButton, SIGNAL(clicked()), this,
                SLOT(addContact()));
    connect(submitButton, SIGNAL(clicked()), this,
                SLOT(submitContact()));
    connect(cancelButton, SIGNAL(clicked()), this,
                SLOT(cancel()));
    //! [signal slot]

    setWindowTitle(tr("Simple Address Book"));
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
