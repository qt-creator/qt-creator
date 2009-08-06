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
