//! [class definition]
#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QWidget>

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

private:
    Ui::AddressBookClass *ui;
};

#endif // ADDRESSBOOK_H
//! [class definition]
