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
    oldName = nameLine->text();
    oldAddress = addressTExt->toPlainText();

    nameLine->clear();
    addressText->clear();

    nameLine->setReadOnly(false);
    nameLine->setFocus(Qt::OtherFocusReason);
    addressText->setReadOnly(false);

    addButton->setEnabled(false);
    submitButton->show();
    cancelButton->show();
}
//! [addContact]

void AddressBook::submitContact()
{
}

void AddressBook::cancel()
{
}
