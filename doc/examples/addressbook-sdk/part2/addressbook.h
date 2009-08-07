#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

//! [include]
#include <QWidget>
#include <QMap>
#include <QMessageBox>
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
    ~AddressBook();

//! [slot definition]
public slots:
    void addContact();
    void submitContact();
    void cancel();
//! [slot definition]

private:
    Ui::AddressBook *ui;

//! [members]
    QMap<QString, QString> contacts;
    QString oldName;
    QString oldAddress;
//! [members]
};

#endif // ADDRESSBOOK_H
