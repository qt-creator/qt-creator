#include "addressbook.h"
#include "ui_addressbook.h"

AddressBook::AddressBook(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddressBook)
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
    oldAddress = addressText->toPlainText();

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

//! [submitContact part1]
void AddressBook::submitContact()
{
    QString name = nameLine->text();
    QString address = addressText->toPlainText();

    if (name == "" || address == "") {
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
        return;
    } else {
        QMessageBox::information(this, tr("Add Unsuccessful"),
            tr("Sorry, \"%1\" is already in your address book.").arg(name));
        return;
    }
//! [submitContact part2]

//! [submitContact part3]
    if (contacts.isEmpty()) {
        nameLine->clear();
        addressText->clear();
    }

    nameLine->setReadOnly(true);
    addressText->setReadOnly(true);
    addButton->setEnabled(true);
    submitButton->hide();
    cancelButton->hide();
}
//! [submitContact part3]

//! [cancel]
void AddressBook::cancel()
{
    nameLine->setText(oldName);
    nameLine->setReadOnly(true);

    addressText->setText(oldAddress);
    addressText->setReadOnly(true);

    addButton->setEnabled(true);
    submitButton->hide();
    cancelButton->hide();
}
//! [cancel]
