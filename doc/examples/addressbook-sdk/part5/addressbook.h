#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QtGui/QWidget>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>
#include <QtGui/QTextEdit>
#include <QtGui/QMessageBox>
//! [include]
#include "finddialog.h"
//! [include]

namespace Ui
{
    class AddressBook;
}

class AddressBook : public QWidget
{
    Q_OBJECT

public:
    AddressBook(QWidget *parent = 0);
    enum Mode { NavigationMode, AddingMode, EditingMode };
    ~AddressBook();

public slots:
    void addContact();
    void submitContact();
    void cancel();
    void editContact();
    void removeContact();
    void next();
    void previous();
//! [findContact]
    void findContact();
//! [findContact]

private:
    Ui::AddressBook *ui;

    void updateInterface(Mode mode);
    QPushButton *addButton;
    QPushButton *submitButton;
    QPushButton *cancelButton;
    QPushButton *editButton;
    QPushButton *removeButton;
    QPushButton *nextButton;
    QPushButton *previousButton;
//! [findButton]
    QPushButton *findButton;
//! [findButton]
    QLineEdit *nameLine;
    QTextEdit *addressText;

    QMap<QString, QString> contacts;
    QString oldName;
    QString oldAddress;
    Mode currentMode;
//! [dialog]
    FindDialog *dialog;
//! [dialog]
};

#endif // ADDRESSBOOK_H
