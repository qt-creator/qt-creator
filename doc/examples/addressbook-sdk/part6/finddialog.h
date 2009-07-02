#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QtGui/QDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>

namespace Ui {
    class FindDialog;
}

class FindDialog : public QDialog {
    Q_OBJECT
public:
    FindDialog(QWidget *parent = 0);
    ~FindDialog();
    QString getFindText();

public slots:
    void findClicked();

private:
    Ui::FindDialog *m_ui;
    QPushButton *findButton;
    QLineEdit *lineEdit;
    QString findText;
};

#endif // FINDDIALOG_H
