#ifndef BRANCHMODEL_H
#define BRANCHMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QVariant>

namespace Git {
    namespace Internal {

class GitClient;

/* A read-only model to list git remote branches in a simple list of branch names.
 * The tooltip describes the latest commit (delayed creation).
 * Provides virtuals to be able to derive a local branch model with the notion
 * of a "current branch". */

class RemoteBranchModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit RemoteBranchModel(GitClient *client, QObject *parent = 0);

    virtual bool refresh(const QString &workingDirectory, QString *errorMessage);

    QString branchName(int row) const;

    // QAbstractListModel
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    int branchCount() const;

    QString workingDirectory() const;
    int findBranchByName(const QString &name) const;

protected:
    struct Branch {
        bool parse(const QString &line, bool *isCurrent);

        QString name;
        QString currentSHA;
        mutable QString toolTip;
    };
    typedef QList<Branch> BranchList;

    /* Parse git output and populate m_branches. */
    bool refreshBranches(const QString &workingDirectory, bool remoteBranches,
                         int *currentBranch, QString *errorMessage);
    bool runGitBranchCommand(const QString &workingDirectory, const QStringList &additionalArgs, QString *output, QString *errorMessage);

private:
    QString toolTip(const QString &sha) const;

    const Qt::ItemFlags m_flags;

    GitClient *m_client;
    QString m_workingDirectory;
    BranchList m_branches;
};

/* LocalBranchModel: Extends RemoteBranchModel by a read-only
 * checkable column indicating the current branch. Provides an
 * editable "Type here" new-branch-row at the bottom to create
 * a new branch. */

class LocalBranchModel : public RemoteBranchModel {
    Q_OBJECT
public:

    explicit LocalBranchModel(GitClient *client,
                         QObject *parent = 0);

    virtual bool refresh(const QString &workingDirectory, QString *errorMessage);

    // is this the "type here" row?
    bool isNewBranchRow(int row) const;
    bool isNewBranchRow(const QModelIndex & index) const { return isNewBranchRow(index.row()); }

    int currentBranch() const;

    // QAbstractListModel
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

signals:
    void newBranchCreated(const QString &);
    void newBranchEntered(const QString &);

private slots:
    void slotNewBranchDelayedRefresh();

private:
    bool checkNewBranchName(const QString &name) const;

    const QVariant m_typeHere;
    const QVariant m_typeHereToolTip;

    int m_currentBranch;
    QString m_newBranch;
};

}
}

#endif // BRANCHMODEL_H
