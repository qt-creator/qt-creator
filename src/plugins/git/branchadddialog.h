#ifndef BRANCHADDDIALOG_H
#define BRANCHADDDIALOG_H

#include <QDialog>

namespace Git {
namespace Internal {


namespace Ui {
    class BranchAddDialog;
}

class BranchAddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BranchAddDialog(QWidget *parent = 0);
    ~BranchAddDialog();

    void setBranchName(const QString &);
    QString branchName() const;

    void setTrackedBranchName(const QString &name, bool remote);

    bool track();

private:
    Ui::BranchAddDialog *m_ui;
};

} // namespace Internal
} // namespace Git

#endif // BRANCHADDDIALOG_H
