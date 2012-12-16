#ifndef BRANCHCHECKOUTDIALOG_H
#define BRANCHCHECKOUTDIALOG_H

#include <QDialog>

namespace Git {
namespace Internal {

namespace Ui {
    class BranchCheckoutDialog;
}

class BranchCheckoutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BranchCheckoutDialog(QWidget *parent, const QString& currentBranch,
                                  const QString& nextBranch);
    ~BranchCheckoutDialog();

    void foundNoLocalChanges();
    void foundStashForNextBranch();

    bool makeStashOfCurrentBranch();
    bool moveLocalChangesToNextBranch();
    bool discardLocalChanges();
    bool popStashOfNextBranch();

    bool hasStashForNextBranch();
    bool hasLocalChanges();

private slots:
    void updatePopStashCheckBox(bool moveChangesChecked);

private:
    Ui::BranchCheckoutDialog *m_ui;
    bool m_foundStashForNextBranch;
    bool m_hasLocalChanges;
};

} // namespace Internal
} // namespace Git

#endif // BRANCHCHECKOUTDIALOG_H
