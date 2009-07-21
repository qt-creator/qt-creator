#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QtGui/QWidget>
#include <QtGui/QMessageBox>
#include <QtCore/QMap>
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
//! [slot definition]
    void findContact();
//! [slot definition]

private:
    Ui::AddressBook *ui;
    void updateInterface(Mode mode);

    QMap<QString, QString> contacts;
    QString oldName;
    QString oldAddress;
    Mode currentMode;
//! [dialog]
    FindDialog *dialog;
//! [dialog]
};

#endif // ADDRESSBOOK_H
