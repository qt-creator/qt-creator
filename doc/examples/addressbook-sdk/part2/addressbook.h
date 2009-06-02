//! [class definition]
#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QtGui/QWidget>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>
#include <QtGui/QTextEdit>

namespace Ui
{
    class AddressBookClass;
}

class AddressBook : public QWidget
{
    Q_OBJECT

public:
    AddressBook(QWidget *parent = 0);
    ~AddressBook();

//! [slot definition]
public slots:
    void addContact();
    void submitContact();
    void cancel();
//! [slot definition]

private:
    Ui::AddressBookClass *ui;

//! [members1]
    QPushButton *addButton;
    QPushButton *submitButton;
    QPushButton *cancelButton;
    QLineEdit *nameLine;
    QTextEdit *addressText;
//! [members1]

//! [members2]
    QMap<QString, QString> contacts;
    QString oldName;
    QString oldAddress;
//! [members2]
};

#endif // ADDRESSBOOK_H
//! [class definition]
