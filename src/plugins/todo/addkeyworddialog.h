#ifndef ADDKEYWORDDIALOG_H
#define ADDKEYWORDDIALOG_H

#include <QDialog>

namespace Ui {
    class AddKeywordDialog;
}

class AddKeywordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddKeywordDialog(QWidget *parent = 0);
    ~AddKeywordDialog();
    QString keywordName();
    QColor keywordColor();
    QIcon keywordIcon();

public slots:
    void chooseColor();

private:
    Ui::AddKeywordDialog *ui;
};

#endif // ADDKEYWORDDIALOG_H
