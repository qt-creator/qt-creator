#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QtGui/QWidget>

namespace Ui
{
    class AddressBook;
}

class AddressBook : public QWidget
{
    Q_OBJECT

public:
    AddressBook(QWidget *parent = 0);
    ~AddressBook();

private:
    Ui::AddressBook *ui;
};

#endif // ADDRESSBOOK_H
