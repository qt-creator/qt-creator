#ifndef S60CERTIFICATEDETAILSDIALOG_H
#define S60CERTIFICATEDETAILSDIALOG_H

#include <QDialog>

namespace Ui {
    class S60CertificateDetailsDialog;
}

class S60CertificateDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit S60CertificateDetailsDialog(QWidget *parent = 0);
    ~S60CertificateDetailsDialog();

    void setText(const QString &text);

private:
    Ui::S60CertificateDetailsDialog *ui;
};

#endif // S60CERTIFICATEDETAILSDIALOG_H
