#ifndef REVERTDIALOG_H
#define REVERTDIALOG_H

#include "ui_revertdialog.h"

#include <QtGui/QDialog>


namespace Mercurial {
namespace Internal {

class mercurialPlugin;

class RevertDialog : public QDialog
{
    Q_OBJECT
public:
    RevertDialog(QWidget *parent = 0);
    ~RevertDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::RevertDialog *m_ui;
    friend class MercurialPlugin;
};

} // namespace Internal
} // namespace Mercurial
#endif // REVERTDIALOG_H
