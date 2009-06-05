//! [class definition]
#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

#include <QtGui/QWidget>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>
#include <QtGui/QTextEdit>
#include <QtGui/QMessageBox>


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

    QPushButton *addButton;
    QPushButton *submitButton;
    QPushButton *cancelButton;
//! [members]
    QPushButton *nextButton;
    QPushButton *previousButton;
//! [members]
    QLineEdit *nameLine;
    QTextEdit *addressText;

    QMap<QString, QString> contacts;
    QString oldName;
    QString oldAddress;
};

#endif // ADDRESSBOOK_H
//! [class definition]
