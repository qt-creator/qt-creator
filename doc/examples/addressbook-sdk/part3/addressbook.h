#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QWidget>
#include <QMessageBox>
#include <QMap>

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

public slots:
    void addContact();
    void submitContact();
    void cancel();
//! [slot definition]
    void next();
    void previous();
//! [slot definition]

private:
    Ui::AddressBook *ui;
    QMap<QString, QString> contacts;
    QString oldName;
    QString oldAddress;
};

#endif // ADDRESSBOOK_H
