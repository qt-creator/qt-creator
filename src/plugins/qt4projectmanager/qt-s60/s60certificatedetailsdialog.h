#ifndef S60CERTIFICATEDETAILSDIALOG_H
#define S60CERTIFICATEDETAILSDIALOG_H

#include <QDialog>

struct S60CertificateDetailsDialogPrivate;

namespace Qt4ProjectManager {
namespace Internal {

class S60CertificateDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit S60CertificateDetailsDialog(QWidget *parent = 0);
    ~S60CertificateDetailsDialog();

    void setText(const QString &text);

private:
    S60CertificateDetailsDialogPrivate *m_d;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60CERTIFICATEDETAILSDIALOG_H
