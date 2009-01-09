#ifndef BRANCHDIALOG_H
#define BRANCHDIALOG_H

#include <QtGui/QDialog>


QT_BEGIN_NAMESPACE
class QPushButton;
class QModelIndex;
QT_END_NAMESPACE

namespace Git {
    namespace Internal {
        namespace Ui {
            class BranchDialog;
        }

        class GitClient;
        class LocalBranchModel;
        class RemoteBranchModel;

        /* Branch dialog: Display a list of local branches at the top
         * and remote branches below. Offers to checkout/delete local
         * branches.
         * TODO: Add new branch (optionally tracking a remote one).
         * How to find out that a local branch is a tracking one? */
        class BranchDialog : public QDialog {
            Q_OBJECT
            Q_DISABLE_COPY(BranchDialog)
        public:
            explicit BranchDialog(QWidget *parent = 0);

            bool init(GitClient *client, const QString &workingDirectory, QString *errorMessage);

            virtual ~BranchDialog();

        protected:
            virtual void changeEvent(QEvent *e);

        private slots:
            void slotEnableButtons();
            void slotCheckoutSelectedBranch();
            void slotDeleteSelectedBranch();
            void slotLocalBranchActivated();
            void slotRemoteBranchActivated(const QModelIndex &);
            void slotCreateLocalBranch(const QString &branchName);

        private:
            bool ask(const QString &title, const QString &what, bool defaultButton);
            void selectLocalBranch(const QString &b);

            int selectedLocalBranchIndex() const;
            int selectedRemoteBranchIndex() const;

            GitClient *m_client;
            Ui::BranchDialog *m_ui;
            QPushButton *m_checkoutButton;
            QPushButton *m_deleteButton;

            LocalBranchModel *m_localModel;
            RemoteBranchModel *m_remoteModel;
            QString m_repoDirectory;
        };
    } // namespace Internal
} // namespace Git
#endif // BRANCHDIALOG_H
