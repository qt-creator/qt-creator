#include "addressbook.h"
#include "ui_addressbook.h"

AddressBook::AddressBook(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddressBook)
{
    ui->setupUi(this);

    ui->nameLine->setReadOnly(true);
    ui->addressText->setReadOnly(true);
    ui->addButton = ui->addButton;
    ui->submitButton->hide();
    ui->cancelButton->hide();

//! [setup fields]
    ui->nextButton->setEnabled(false);
    ui->previousButton->setEnabled(false);
//! [setup fields]

    connect(ui->addButton, SIGNAL(clicked()), this,
                SLOT(addContact()));
    connect(ui->submitButton, SIGNAL(clicked()), this,
                SLOT(submitContact()));
    connect(ui->cancelButton, SIGNAL(clicked()), this,
                SLOT(cancel()));
//! [signal slot]
    connect(ui->nextButton, SIGNAL(clicked()), this,
                SLOT(next()));
    connect(ui->previousButton, SIGNAL(clicked()), this,
                SLOT(previous()));
//! [signal slot]

    setWindowTitle(tr("Simple Address Book"));
}

AddressBook::~AddressBook()
{
    delete ui;
}

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
//! [disable navigation]
    ui->nextButton->setEnabled(false);
    ui->previousButton->setEnabled(false);
//! [disable navigation]
    ui->submitButton->show();
    ui->cancelButton->show();
}

void AddressBook::submitContact()
{
    QString name = ui->nameLine->text();
    QString address = ui->addressText->toPlainText();

    if (name == "" || address == "") {
        QMessageBox::information(this, tr("Empty Field"),
            tr("Please enter a name and address."));
        return;
    }

    if (!contacts.contains(name)) {
        contacts.insert(name, address);
        QMessageBox::information(this, tr("Add Successful"),
            tr("\"%1\" has been added to your address book.").arg(name));
    } else {
        QMessageBox::information(this, tr("Add Unsuccessful"),
            tr("Sorry, \"%1\" is already in your address book.").arg(name));
        return;
    }

    if (contacts.isEmpty()) {
        ui->nameLine->clear();
        ui->addressText->clear();
    }

    ui->nameLine->setReadOnly(true);
    ui->addressText->setReadOnly(true);
    ui->addButton->setEnabled(true);

//! [enable navigation]
    int number = contacts.size();
    ui->nextButton->setEnabled(number > 1);
    ui->previousButton->setEnabled(number > 1);
//! [enable navigation]
    ui->submitButton->hide();
    ui->cancelButton->hide();
}

void AddressBook::cancel()
{
    ui->nameLine->setText(oldName);
    ui->nameLine->setReadOnly(true);

    ui->addressText->setText(oldAddress);
    ui->addressText->setReadOnly(true);
    ui->addButton->setEnabled(true);

    int number = contacts.size();
    ui->nextButton->setEnabled(number > 1);
    ui->previousButton->setEnabled(number > 1);

    ui->submitButton->hide();
    ui->cancelButton->hide();
}

//! [next]
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
//! [next]

//! [previous]
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
//! [previous]

