#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QWidget>
#include <QMessageBox>
#include <QMap>
#include "finddialog.h"

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
    void findContact();
//! [slot definition]
    void saveToFile();
    void loadFromFile();
//! [slot definition]

private:
    Ui::AddressBook *ui;
    void updateInterface(Mode mode);

    QMap<QString, QString> contacts;
    QString oldName;
    QString oldAddress;
    Mode currentMode;
    FindDialog *dialog;
};

#endif // ADDRESSBOOK_H
