#ifndef SRCDESTDIALOG_H
#define SRCDESTDIALOG_H

#include <QtGui/QDialog>
#include <utils/pathchooser.h>

namespace Mercurial {
namespace Internal {

namespace Ui {
class SrcDestDialog;
}

class SrcDestDialog : public QDialog
{
    Q_OBJECT
public:

    SrcDestDialog(QWidget *parent = 0);
    ~SrcDestDialog();
    void setPathChooserKind(Core::Utils::PathChooser::Kind kind);
    QString getRepositoryString();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::SrcDestDialog *m_ui;
};

} // namespace Internal
} // namespace Mercurial
#endif // SRCDESTDIALOG_H
