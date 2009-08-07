#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

namespace Ui {
    class FindDialog;
}

class FindDialog : public QDialog {
    Q_OBJECT
public:
    FindDialog(QWidget *parent = 0);
    ~FindDialog();
//! [findText]
    QString findText();
//! [findText]

//! [findClicked]
public slots:
    void findClicked();
//! [findClicked]

//! [private members]
private:
    Ui::FindDialog *m_ui;
//! [private members]
};

#endif // FINDDIALOG_H
