#include "addressbook.h"
#include "ui_addressbook.h"

AddressBook::AddressBook(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddressBook)
{
    ui->setupUi(this);

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

    nextButton = new QPushButton;
    nextButton = ui->nextButton;
    nextButton->setEnabled(false);

    previousButton = new QPushButton;
    previousButton = ui->previousButton;
    previousButton->setEnabled(false);

//! [extract objects]
    editButton = new QPushButton;
    editButton = ui->editButton;
    editButton->setEnabled(false);

    removeButton = new QPushButton;
    removeButton = ui->removeButton;
    removeButton->setEnabled(false);
//! [extract objects]

    connect(addButton, SIGNAL(clicked()), this,
                SLOT(addContact()));
    connect(submitButton, SIGNAL(clicked()), this,
                SLOT(submitContact()));
    connect(cancelButton, SIGNAL(clicked()), this,
                SLOT(cancel()));
    connect(nextButton, SIGNAL(clicked()), this,
                SLOT(next()));
    connect(previousButton, SIGNAL(clicked()), this,
                SLOT(previous()));
//! [signal slot]
    connect(editButton, SIGNAL(clicked()), this,
                SLOT(editContact()));
    connect(removeButton, SIGNAL(clicked()), this,
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
    oldName = nameLine->text();
    oldAddress = addressText->toPlainText();

    nameLine->clear();
    addressText->clear();

    updateInterface(AddingMode);
}
//! [addContact]

//! [submitContact part1]
void AddressBook::submitContact()
{
//! [submitContact part1]
    QString name = nameLine->text();
    QString address = addressText->toPlainText();

    if (name == "" || address == "") {
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
    nameLine->setText(oldName);
    nameLine->setReadOnly(true);

    updateInterface(NavigationMode);
}
//! [cancel]

void AddressBook::next()
{
    QString name = nameLine->text();
    QMap<QString, QString>::iterator i = contacts.find(name);

    if (i != contacts.end())
        i++;
    if (i == contacts.end())
        i = contacts.begin();

    nameLine->setText(i.key());
    addressText->setText(i.value());
}

void AddressBook::previous()
{
    QString name = nameLine->text();
    QMap<QString, QString>::iterator i = contacts.find(name);

    if (i == contacts.end()) {
        nameLine->clear();
        addressText->clear();
        return;
    }

    if (i == contacts.begin())
        i = contacts.end();

    i--;
    nameLine->setText(i.key());
    addressText->setText(i.value());
}

//! [editContact]
void AddressBook::editContact()
{
    oldName = nameLine->text();
    oldAddress = addressText->toPlainText();

    updateInterface(EditingMode);
}
//! [editContact]

//! [removeContact]
void AddressBook::removeContact()
{
    QString name = nameLine->text();
    QString address = addressText->toPlainText();

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

        nameLine->setReadOnly(false);
        nameLine->setFocus(Qt::OtherFocusReason);
        addressText->setReadOnly(false);

        addButton->setEnabled(false);
        editButton->setEnabled(false);
        removeButton->setEnabled(false);

        nextButton->setEnabled(false);
        previousButton->setEnabled(false);

        submitButton->show();
        cancelButton->show();
        break;
//! [updateInterface part1]

//! [updateInterface part2]
    case NavigationMode:

        if (contacts.isEmpty()) {
            nameLine->clear();
            addressText->clear();
        }

        nameLine->setReadOnly(true);
        addressText->setReadOnly(true);
        addButton->setEnabled(true);

        int number = contacts.size();
        editButton->setEnabled(number >= 1);
        removeButton->setEnabled(number >= 1);
        nextButton->setEnabled(number > 1);
        previousButton->setEnabled(number >1);

        submitButton->hide();
        cancelButton->hide();
        break;
    }
}
//! [updateInterface part2]
